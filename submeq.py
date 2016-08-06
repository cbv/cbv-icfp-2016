#!/usr/bin/python

import sys
import subprocess
import os

problem_number = int(sys.argv[1])
outproblemname = "problem{0:06d}".format(problem_number)
subfile = os.path.join("reptiloid-db", outproblemname, "best_submitted")

if not os.access(subfile, os.F_OK):
    print "Problem not already submitted"
    exit(1)

fp = os.stat("problems/prob" + str(problem_number))

print "OK"

for f in os.listdir("problems/"):
    if f == "problems/prob" + str(problem_number):
        continue
    try:
        fi = os.stat(os.path.join("problems", f))
    except OSError:
        continue

    if fp.st_size <> fi.st_size:
        continue

    try:
        out = subprocess.check_output(["diff",
                                       "problems/prob" + str(problem_number),
                                       os.path.join("problems", f)],
                                      stderr = subprocess.STDOUT)
    except subprocess.CalledProcessError:
        continue
    if len(out) == 0:
        print "Submitting", f
        subprocess.call(["./reptiloid.py", f.lstrip("prob"),
                         str(subfile)])
