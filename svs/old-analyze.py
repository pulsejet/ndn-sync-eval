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

RUN_NUMBER_VALS = list(range(1, 11))
NUM_NODES_VALS = [5,10,15,20,25,30,35,40]

LOG_PREFIX = "GEANT_L10"

EPOCH = datetime.now().timestamp()

headp = False

ckeys = ['num_nodes', 'run_number', 'node', 'succ', 'pm_succ', 'avg',
         'median', 'max', 'sync_ints']
dict_writer = csv.DictWriter(sys.stdout, ckeys)
dict_writer.writeheader()

FULL_CSV = "{}.csv".format(LOG_PREFIX)
with open(FULL_CSV, 'w') as f:
    fdict_writer = csv.DictWriter(f, ckeys)
    fdict_writer.writeheader()

for num_nodes in NUM_NODES_VALS:
    NODES_AVG = []

    NODE_CSV = "{}-{}.csv".format(LOG_PREFIX, num_nodes)
    with open(NODE_CSV, 'w') as f:
        fdict_writer = csv.DictWriter(f, ckeys)
        fdict_writer.writeheader()

    for run_number in RUN_NUMBER_VALS:
        RUN_NUMBER = run_number
        NUM_NODES = num_nodes

        LOG_NAME = "{}-{}-{}".format(LOG_PREFIX, NUM_NODES, RUN_NUMBER)
        LOG_DIR = "/home/vagrant/mini-ndn/work/log/svs/" + LOG_NAME

        logfiles = glob.glob(LOG_DIR + "/*.log")

        PUBLISHES = {}
        SYNCS = {}
        RECIEVES = {}
        SYNC_INTS = defaultdict(int)

        for logfile in logfiles:
            nodename = logfile.split('/')[-1].split('.')[0]
            df = pd.read_csv(logfile, header=None, names=['t', 'pid', 'tid', 'm'], skipinitialspace = True)
            df = df.replace(' "', '', regex=True).replace('"', '', regex=True)

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
                    SYNC_INTS[nodename] += 1

        TIMINGS = {}

        for msg in RECIEVES:
            if msg not in PUBLISHES:
                print(msg, 'was received and never published :)')
                exit(1)

            pubtime = PUBLISHES[msg]
            recv_times = RECIEVES[msg]['times']

            deltas = [x - pubtime for x in recv_times]
            publisher = msg.split('/')[3]

            if publisher not in TIMINGS:
                TIMINGS[publisher] = []
            TIMINGS[publisher].extend(deltas)

        DELTAS = []
        for node in TIMINGS:
            deltas = TIMINGS[node]
            med_d = statistics.median(deltas)
            avg_d = sum(deltas) / len(deltas)
            max_d = max(deltas)
            DELTAS.append({
                'num_nodes': num_nodes,
                'run_number': run_number,
                'node': node,
                'succ': len(deltas),
                'pm_succ': round(len(deltas) / (24 * (NUM_NODES - 1)), 3),
                'avg': round(avg_d),
                'median': round(med_d),
                'max': round(max_d),
                'sync_ints': SYNC_INTS[node],
            })

        T_SUCC = [D['succ'] for D in DELTAS]
        T_AVG = [D['avg'] for D in DELTAS]
        T_MED = [D['median'] for D in DELTAS]
        T_MAX = [D['max'] for D in DELTAS]
        T_SYNC_INTS = [D['sync_ints'] for D in DELTAS]
        NODES_AVG.append({
            'num_nodes': num_nodes,
            'run_number': run_number,
            'node': 'AVG',
            'succ': round(sum(T_SUCC) / len(T_SUCC), 1),
            'pm_succ': round(sum(T_SUCC) / (24 * (NUM_NODES - 1) * NUM_NODES), 3),
            'avg': round(sum(T_AVG) / len(T_AVG)),
            'median': round(sum(T_MED) / len(T_MED)),
            'max': round(sum(T_MAX) / len(T_MAX)),
            'sync_ints': round(sum(T_SYNC_INTS) / len(T_SYNC_INTS)),
        })
        dict_writer.writerows(DELTAS)

        with open(NODE_CSV, 'a') as f:
            fdict_writer = csv.DictWriter(f, ckeys)
            fdict_writer.writerows(DELTAS)

    # ============
    T_SUCC = [D['succ'] for D in NODES_AVG]
    T_AVG = [D['avg'] for D in NODES_AVG]
    T_MED = [D['median'] for D in NODES_AVG]
    T_MAX = [D['max'] for D in NODES_AVG]
    T_SYNC_INTS = [D['sync_ints'] for D in NODES_AVG]
    obj = {
        'num_nodes': num_nodes,
        'run_number': 'AVG',
        'node': 'AVG',
        'succ': round(sum(T_SUCC) / len(T_SUCC), 1),
        'pm_succ': round(sum(T_SUCC) / (24 * (NUM_NODES - 1) * len(RUN_NUMBER_VALS)), 3),
        'avg': round(sum(T_AVG) / len(T_AVG)),
        'median': round(sum(T_MED) / len(T_MED)),
        'max': round(sum(T_MAX) / len(T_MAX)),
        'sync_ints': round(sum(T_SYNC_INTS) / len(T_SYNC_INTS)),
    }
    NODES_AVG.append(obj)
    dict_writer.writerows(NODES_AVG)

    with open(FULL_CSV, 'a') as f:
        fdict_writer = csv.DictWriter(f, ckeys)
        if RUN_NUMBER == 1:
            fdict_writer.writeheader()
        fdict_writer.writerows(NODES_AVG)
