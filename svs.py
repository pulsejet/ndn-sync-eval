# -*- Mode:python; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
#
# Copyright (C) 2015-2020, The University of Memphis,
#                          Arizona Board of Regents,
#                          Regents of the University of California.
#
# This file is part of Mini-NDN.
# See AUTHORS.md for a complete list of Mini-NDN authors and contributors.
#
# Mini-NDN is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Mini-NDN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Mini-NDN, e.g., in COPYING.md file.
# If not, see <http://www.gnu.org/licenses/>.

import random
import time
import configparser
import psutil
import os
from collections import defaultdict

from mininet.log import setLogLevel, info

from minindn.apps.application import Application

from minindn.helpers.nfdc import Nfdc
from minindn.minindn import Minindn
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.ndn_routing_helper import NdnRoutingHelper

RUN_NUMBER = 0
NUM_NODES = 20
PUB_TIMING = 0

PUB_TIMING_VALS = [1000, 5000, 10000, 15000]
RUN_NUMBER_VALS = list(range(1, 4))
LOG_PREFIX = "GEANT"
#topoFile = "topologies/default-topology.conf"
topoFile = "topologies/geant.conf"

#SYNC_EXEC = "/home/vagrant/mini-ndn/work/ndn-svs/build/examples/eval"
SYNC_EXEC = "/home/vagrant/mini-ndn/work/ChronoSync/build/examples/eval"
LOG_MAIN_DIRECTORY = "/home/vagrant/mini-ndn/work/log/chronosync/"

def getLogPath():
    LOG_NAME = "{}-{}-{}".format(LOG_PREFIX, PUB_TIMING, RUN_NUMBER)
    logpath = LOG_MAIN_DIRECTORY + LOG_NAME

    if not os.path.exists(logpath):
        os.makedirs(logpath)
        os.chown(logpath, 1000, 1000)

    return logpath

class SvsChatApplication(Application):
    """
    Wrapper class to run the chat application from each node
    """
    def get_svs_identity(self):
        return "/ndn/{0}-site/{0}/svs_chat/{0}".format(self.node.name)

    def start(self):
        exe = SYNC_EXEC
        identity = self.get_svs_identity()

        run_cmd = "{} {} {}/{}.log {} >/dev/null 2>&1 &".format(exe, identity, getLogPath(), self.node.name, PUB_TIMING)
        ret = self.node.cmd(run_cmd)
        info("[{}] running {} == {}\n".format(self.node.name, run_cmd, ret))

def count_running(pids):
    return sum(psutil.pid_exists(pid) for pid in pids)

def get_pids():
    pids = []
    for proc in psutil.process_iter():
        try:
            pinfo = proc.as_dict(attrs=['pid', 'name', 'create_time'])
            # Check if process name contains the given name string.
            if "eval" in pinfo['name'].lower():
                pids.append(pinfo['pid'])
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
    return pids

if __name__ == '__main__':
    setLogLevel('info')

    Minindn.cleanUp()
    Minindn.verifyDependencies()

    ndn = Minindn(topoFile=topoFile)

    ndn.start()

    info('Starting NFD on nodes\n')
    nfds = AppManager(ndn, ndn.net.hosts, Nfd)
    info('Sleeping 10 seconds\n')
    time.sleep(10)

    info('Setting NFD strategy to multicast on all nodes with prefix')
    for node in ndn.net.hosts:
        Nfdc.setStrategy(node, "/ndn/svs", Nfdc.STRATEGY_MULTICAST)
        Nfdc.setStrategy(node, "/ndn/chronosync", Nfdc.STRATEGY_MULTICAST)

    info('Adding static routes to NFD\n')
    start = int(time.time() * 1000)

    grh = NdnRoutingHelper(ndn.net, 'udp', 'link-state')
    for host in ndn.net.hosts:
        grh.addOrigin([ndn.net[host.name]], ["/ndn/svs/", "/ndn/chronosync/"])

    grh.calculateNPossibleRoutes()

    end = int(time.time() * 1000)
    info('Added static routes to NFD in {} ms\n'.format(end - start))
    info('Sleeping 10 seconds\n')
    time.sleep(10)

    for pub_timing in PUB_TIMING_VALS:
        for run_number in RUN_NUMBER_VALS:
            RUN_NUMBER = run_number
            PUB_TIMING = pub_timing

            # Clear content store
            for node in ndn.net.hosts:
                cmd = 'nfdc cs erase /'
                info(node.cmd(cmd))

                with open("{}/report-start-{}.status".format(getLogPath(), node.name), "w") as f:
                    f.write(node.cmd('nfdc status report'))

            time.sleep(1)

            random.seed(RUN_NUMBER)
            allowed_hosts = [x for x in ndn.net.hosts if len(x.intfList()) < 8]
            pub_hosts = random.sample(allowed_hosts, NUM_NODES)

            # ================= SVS BEGIN ====================================

            # identity_app = AppManager(ndn, pub_hosts, IdentityApplication)
            svs_chat_app = AppManager(ndn, pub_hosts, SvsChatApplication)

            # =================== SVS END ====================================

            pids = get_pids()
            info("pids: {}\n".format(pids))
            count = count_running(pids)
            while count > 0:
                info("{} nodes are runnning\n".format(count))
                time.sleep(5)
                count = count_running(pids)

            for node in ndn.net.hosts:
                with open("{}/report-end-{}.status".format(getLogPath(), node.name), "w") as f:
                    f.write(node.cmd('nfdc status report'))

    ndn.stop()
