#!/usr/bin/env python

import requests
import time
import os
import sys
import subprocess

api_key = "129-7af108ba768db1f95140f91e3c4d34e4"

headers = {'Expect': '', 'X-API-Key': api_key}

def get_blob(hash):
    r = requests.get('http://2016sv.icfpcontest.org/api/blob/' + hash, headers = headers)
    if r.status_code != 200:
        print("ERROR")
        print(r)
        exit(1)
    else:
        return r

def get_problems():
    response = requests.get('http://2016sv.icfpcontest.org/api/snapshot/list', headers = headers)
    if response.status_code != 200:
        print("error response")
        print(response)
        exit(1)
    snapshot_hash = sorted(response.json()['snapshots'], key=lambda s: -s['snapshot_time'])[0]['snapshot_hash']
    time.sleep(1)
    s = get_blob(snapshot_hash)
    response_json = s.json()
    return response_json['problems']

def perfect_sols(prob):
    count = 0
    for sol in prob['ranking']:
        if sol['resemblance'] == 1.0:
            count += 1
    return count

problems = get_problems()

print("Calculating base scores")

for prob in problems:
    prob['base_score'] = prob['solution_size'] / (perfect_sols(prob) + 2)
    ex_pts = (5000 - prob['solution_size']) / (perfect_sols(prob) + 1)
    new_pts = (5000 - prob['solution_size']) / (perfect_sols(prob) + 2)
    prob['delta_score'] = prob['base_score'] + (ex_pts - new_pts)

print("Calculating equivalence classes")

classes = dict()

def exists(i):
    if i in classes.keys():
        True
    for c in classes.values():
        if i in c:
            return True
    return False

def which_class(i):
    if i in classes.keys():
        return i
    for j in classes.keys():
        if i in classes[j]:
            return j

for ip in range(len(problems)):
    if ip % 100 == 0:
        print(ip)
    i = problems[ip]['problem_id']
    try:
        fi = os.stat("problems/prob" + str(i))
    except OSError:
        continue
    if exists(i):
        continue
    for jp in range(ip + 1, len(problems)):
        j = problems[jp]['problem_id']
        if not exists(j):
            try:
                fj = os.stat("problems/prob" + str(j))
            except OSError:
                continue
            if fi.st_size != fj.st_size:
                continue
            try:
                out = subprocess.check_output(["diff", "problems/prob" + str(i),
                                               "problems/prob" + str(j)],
                                              stderr = subprocess.STDOUT)
            except subprocess.CalledProcessError:
                continue
            if len(out) == 0:
                if i in classes:
                    classes[i].append(j)
                else:
                    classes[i] = [j]

to_solve = []
to_submeq = []

def is_solved(pid):
    outproblemname = "problem{0:06d}".format(pid)
    try:
        subfile = os.path.join("reptiloid-db", outproblemname, "best_submitted")
        best = os.readlink(subfile)
        if best.startswith('solution_1.0000000'):
            return True
    except:
        pass
    return False

def get_prob(pid):
    for prob in problems:
        if prob['problem_id'] == pid:
            return prob

print("Calculating equivalence class scores")

ind = 0

for prob in problems:
    if ind % 100 == 0:
        print(ind)
    ind += 1
    pid = prob['problem_id']
    prob['equiv_score'] = prob['delta_score']
    if is_solved(pid):
        #We've already solved the problem. Can we get points with submeq?
        if pid in classes:
            for o_pid in classes[pid]:
                if not is_solved(o_pid):
                    o_prob = get_prob(o_pid)
                    prob['equiv_score'] += o_prob['delta_score']
            if prob['equiv_score'] > 0:
                to_submeq = to_submeq + [prob]
        elif exists(pid):
            c = which_class(pid)
            for o_pid in classes[c] + [c]:
                if not is_solved(o_pid):
                    o_prob = get_prob(o_pid)
                    prob['equiv_score'] += o_prob['delta_score']
            if prob['equiv_score'] > 0:
                to_submeq.append(prob)
    else:
        #We haven't solved this problem.
        dup_solved = False
        if exists(pid):
            c = which_class(pid)
            for o_pid in classes[c] + [c]:
                if is_solved(o_pid):
                    dup_solved = True
                    break
                o_prob = get_prob(o_pid)
                prob['equiv_score'] += o_prob['delta_score']
        if not dup_solved:
            #We haven't solved a duplicate.
            to_solve.append(prob)

to_solve_sorted = sorted(to_solve, key=lambda p: p['equiv_score'], reverse=True)
to_submeq_sorted = sorted(to_submeq, key=lambda p: p['equiv_score'], reverse=True)

print("Problems to solve:")
print("Problem\t\tPoints\t\tDelta-points\tEquiv-points")
for prob in to_solve_sorted:
    print("\t\t".join([str(prob['problem_id']), str(prob['base_score']),
                     str(prob['delta_score']), str(prob['equiv_score'])]))

print("SOLVED PROBLEMS to submeq:")
print("Problem\t\tPoints\t\tDelta-points\tEquiv-points")
for prob in to_submeq_sorted:
    print("\t\t".join([str(prob['problem_id']), str(prob['base_score']),
                     str(prob['delta_score']), str(prob['equiv_score'])]))
