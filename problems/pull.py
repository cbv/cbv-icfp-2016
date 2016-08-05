#!/usr/bin/python

import requests
import time
import os
import sys

if len(sys.argv) < 2:
    print "Usage: pull.py <snapshot_hash>"
    exit(1)

snapshot = sys.argv[1] #"5316e3b9d3224e6ef99a5220e8f3dcab5e58bb0d"
api_key = "129-7af108ba768db1f95140f91e3c4d34e4"

headers = {'Expect': '', 'X-API-Key': api_key}

def get_blob(hash):
    r = requests.get('http://2016sv.icfpcontest.org/api/blob/' + hash, headers = headers)
    return r

def get_problems(snapshot):
    s = get_blob(snapshot)
    return s.json()['problems']

for prob in get_problems(snapshot):
    pid = prob['problem_id']
    filename = 'prob' + str(pid)
    if os.access(filename, os.F_OK):
        print "Skipping problem " + str(pid)
    else:
        print "Getting problem " + str(pid)
        pblob = get_blob(prob['problem_spec_hash'])
        f = open(filename, 'w')
        f.write(pblob.text)
        f.close()
        time.sleep(1)
