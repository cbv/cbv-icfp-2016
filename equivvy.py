#!/usr/bin/python

import sys
import subprocess
import os

maxno = int(sys.argv[1])

classes = dict()

def exists(i):
    if i in classes.keys():
        True
    for c in classes.values():
        if i in c:
            return True
    return False

for i in range(1, maxno + 1):
    print i
    try:
        fi = os.stat("problems/prob" + str(i))
    except OSError:
        continue
    if exists(i):
        continue
    for j in range(i + 1, maxno + 1):
        if not exists(j):
            try:
                fj = os.stat("problems/prob" + str(j))
            except OSError:
                continue
            if fi.st_size <> fj.st_size:
                continue
            try:
                out = subprocess.check_output(["diff", "problems/prob" + str(i),
                                               "problems/prob" + str(j)],
                                              stderr = subprocess.STDOUT)
            except subprocess.CalledProcessError:
                continue
            if len(out) == 0:
                if i in classes:
                    classes[i] = classes[i] + [j]
                else:
                    classes[i] = [j]

print classes
