#!/usr/bin/env python

import requests
import time
import os
import sys
import subprocess
import re


problems = []
for root, dirs, files in os.walk('problems'):
    for f in files:
        if re.match(r'prob\d+',f):
            problems.append(int(f[4:]))



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
    i = problems[ip]
    try:
        fi = os.stat("problems/prob" + str(i))
    except OSError:
        continue
    if exists(i):
        continue
    for jp in range(ip + 1, len(problems)):
        j = problems[jp]
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

print("Calculating equivalence class scores")

ind = 0

for c in classes:
    have_unsolved = False
    have_solved = None
    for pid in c:
        if is_solved(pid):
            have_solved = pid
        else:
            have_unsolved = True
    if have_unsolved and have_solved is not None:
        print("Submeq " + str(have_solved))
