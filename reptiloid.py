#!/usr/bin/env python3

import argparse
import hashlib
import os
import re
import requests
import shutil
import subprocess
import sys
import time

CHECKER_BINARY = "./cpp-check/check"

API_KEY = "129-7af108ba768db1f95140f91e3c4d34e4"
HEADERS = {'Expect': '', 'X-API-Key': API_KEY}

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Check and possibly submit a solution.")
    parser.add_argument("problem_number", type=int, help="problem number")
    parser.add_argument("solution", help="solution file")
    args = parser.parse_args()

    #if sys.stdin.isatty():
    #    eprint("waiting for solution on stdin...")

    problem_number = args.problem_number
    solfile = args.solution
    probfile = "problems/prob{}".format(problem_number)

    if not(os.path.isfile(CHECKER_BINARY)):
        eprint("Solution checker {} does not exist. Perhaps you need to call `make`?".format(CHECKER_BINARY))
        exit(1)

    if not(os.path.isfile(probfile)):
        eprint("Problem file {} does not exist.".format(probfile))
        exit(1)

    if not(os.path.isfile(solfile)):
        eprint("Solution file {} does not exist.".format(solfile))
        exit(1)

    completed_process = subprocess.run([CHECKER_BINARY, probfile, solfile], stdout=subprocess.PIPE)
    if completed_process.returncode != 0:
        eprint("check failed")
        exit(1)

    result = completed_process.stdout.decode("utf-8")
    if not result.startswith("Score:"):
        eprint("Expected output from checker to start with 'Score:'. Instead got {}".format(result))
        exit(1)

    (prefix, suffix) = result[7:].split("~=")

    approx = suffix.strip()

    if approx[0] == '1' and prefix.strip() != "1/1":
        # not exactly 1, so don't round up
        approx = "0.9999999"

    if len(approx) != 9:
        eprint("expected an approximation with 7 digits to the right of the decimal")
        exit(1)

    approx_float = float(approx)

    outproblemname = "problem{0:06d}".format(problem_number)
    outdir = os.path.join("reptiloid-db", outproblemname)
    if not os.path.isdir(outdir):
        os.mkdir(outdir)

    # read the file and get its hash
    solution_string = open(solfile, "r").read()
    solution_size = len(re.sub(r"\s+", "", solution_string))
    hasher = hashlib.new("sha256")
    hasher.update(solution_string.encode("utf-8"))
    hash = hasher.hexdigest()[:16]

    if solution_size > 5000:
        eprint("Solution is larger than 5000 bytes and therefore invalid.")
        exit(1)

    solution_name = "solution_{}_{}".format(approx, hash)
    outfile = os.path.join(outdir, solution_name)

    # copy the file to the output directory, if it does not already exist there
    if not os.path.isfile(outfile):
        eprint("Copying solution to {}.".format(outfile))
        shutil.copyfile(solfile, outfile)

    symlinkfile = os.path.join(outdir, "best_submitted")

    should_submit = True
    if os.path.islink(symlinkfile):
        old_size = os.stat(symlinkfile).st_size
        new_size = os.stat(outfile).st_size
        old_best_submitted = os.readlink(symlinkfile)
        old_best_score = float(old_best_submitted.split('_')[1])
        if old_best_submitted == solution_name:
            # we've already submitted this
            eprint("We've already submitted this solution.")
            should_submit = False
        elif approx_float < old_best_score:
            eprint("We've already submitted a better solution for this problem.")
            should_submit = False
        elif approx_float == old_best_score and old_size <= new_size:
            eprint("We've already submitted a smaller solution with the same score for this problem.")
            should_submit = False

    if should_submit:
        eprint("submitting...")
        response = requests.post("http://2016sv.icfpcontest.org/api/solution/submit", headers = HEADERS,
                                 data = {"problem_id": problem_number, "solution_spec": solution_string})
        eprint(response)
        if response.status_code != 200:
            eprint("Error from server.")
            exit(1)
        else:
            response_json = response.json()
            eprint(response_json)
            # update the symlink
            tmplink = "tmp.{}".format(int(time.time() * 10))
            os.symlink(solution_name, tmplink)
            os.rename(tmplink, symlinkfile)
            eprint("Success.")







