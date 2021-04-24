#!/bin/python3

import glob
import pandas as pd
import sys
import csv
import json
import statistics
from datetime import datetime
from collections import OrderedDict
from operator import itemgetter
from collections import defaultdict

OUTER_LOG_DIR = "/home/vagrant/mini-ndn/work/log/svs"

NUM_NODES = 20
PUB_TIMING_VALS = [10000, 15000]
RUN_NUMBER_VALS = list(range(1, 4))

LOG_PREFIX = "GEANT_L0"

EPOCH = datetime.now().timestamp()

TIMING_DATA = [[] for x in PUB_TIMING_VALS]
SYNC_INT_DATA = [[] for x in PUB_TIMING_VALS]
SUCCESS_DATA = [[] for x in PUB_TIMING_VALS]
NUM_PUBLISHED_DATA = [[] for x in PUB_TIMING_VALS]

NUM_nInInterests = [[] for x in PUB_TIMING_VALS]
NUM_nOutData = [[] for x in PUB_TIMING_VALS]

for i_t, PUB_TIMING in enumerate(PUB_TIMING_VALS):
    for RUN_NUMBER in RUN_NUMBER_VALS:
        PUBLISHING_NODES = []

        # Let log directory and files
        LOG_NAME = "{}-{}-{}".format(LOG_PREFIX, PUB_TIMING, RUN_NUMBER)
        LOG_DIR = OUTER_LOG_DIR + '/' + LOG_NAME

        PUBLISHES = {}
        SYNCS = {}
        RECIEVES = {}
        SYNC_INTS = 0

        for logfile in glob.glob(LOG_DIR + "/*.log"):
            nodename = logfile.split('/')[-1].split('.')[0]
            df = pd.read_csv(logfile, header=None, names=['t', 'pid', 'tid', 'm'], skipinitialspace = True)
            df = df.replace(' "', '', regex=True).replace('"', '', regex=True)

            print("Read", logfile)

            for index, row in df.iterrows():
                t = datetime.strptime(row['t'], "%Y-%m-%d %H:%M:%S.%f")
                ti = int((t - datetime.utcfromtimestamp(0)).total_seconds() * 1000)
                m = row['m'].split('::')

                if 'PUBL_MSG' in m[0]:
                    PUBLISHES[m[2]] = ti
                if 'RECV_MSG' in m[0]:
                    if m[2] not in RECIEVES:
                        RECIEVES[m[2]] = {'nodes': [], 'times': []}
                    RECIEVES[m[2]]['nodes'].append(m[1])
                    RECIEVES[m[2]]['times'].append(ti)
                if 'SEND_SYNC_INT' in m[0]:
                    SYNC_INTS += 1

        for msg in RECIEVES:
            if msg not in PUBLISHES:
                print(msg, 'was received and never published :)')
                exit(1)

            pubtime = PUBLISHES[msg]
            recv_times = RECIEVES[msg]['times']

            deltas = [x - pubtime for x in recv_times]
            publisher = msg.split('/')[3]
            if publisher not in PUBLISHING_NODES:
                PUBLISHING_NODES.append(publisher)

            TIMING_DATA[i_t].extend(deltas)

        SYNC_INT_DATA[i_t].append(SYNC_INTS / len(PUBLISHES))
        SUCCESS_DATA[i_t].append((len([x for k in RECIEVES for x in RECIEVES[k]['times']]) / (NUM_NODES - 1)) / len(PUBLISHES))
        NUM_PUBLISHED_DATA[i_t].append(len(PUBLISHES))

        nInInterests = 0
        nOutData = 0
        for startfile in glob.glob(LOG_DIR + "/report-start-*.status"):
            nodename = startfile.split('/')[-1].split('.')[0].split('-')[-1]
            endfile = startfile.replace('report-start', 'report-end')

            def read_status_file(filename):
                status = {}
                with open(filename, "r") as sf:
                    for line in sf.readlines():
                        if "Channels" in line:
                            break

                        if "=" in line:
                            line = line.strip()
                            v = line.split('=')
                            try:
                                status[v[0]] = int(v[1])
                            except ValueError:
                                status[v[0]] = v[1]
                return status

            start = read_status_file(startfile)
            end = read_status_file(endfile)

            nInInterests += end['nInInterests'] - start['nInInterests']
            nOutData += end['nOutData'] - start['nOutData']

        NUM_nInInterests[i_t].append(nInInterests)
        NUM_nOutData[i_t].append(nOutData)

print(len(TIMING_DATA))
print(len(TIMING_DATA[0]))
print(SYNC_INT_DATA)
print(SUCCESS_DATA)
print(NUM_PUBLISHED_DATA)
print(NUM_nInInterests)
print(NUM_nOutData)
