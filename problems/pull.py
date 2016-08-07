#!/usr/bin/python

import requests
import time
import os
import sys

api_key = "129-7af108ba768db1f95140f91e3c4d34e4"

headers = {'Expect': '', 'X-API-Key': api_key}

def get_blob(hash):
    r = requests.get('http://2016sv.icfpcontest.org/api/blob/' + hash, headers = headers)
    if r.status_code != 200:
        print "ERROR"
        print r
        exit(1)
    else:
        return r

def get_problems():
    response = requests.get('http://2016sv.icfpcontest.org/api/snapshot/list', headers = headers)
    snapshot_hash = sorted(response.json()['snapshots'], key=lambda s: -s['snapshot_time'])[0]['snapshot_hash']
    time.sleep(1)
    s = get_blob(snapshot_hash)
    response_json = s.json()
    return response_json['problems']

for prob in get_problems():
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
