#!/usr/bin/env python3

import argparse
import os
from subprocess import call
import sys

CHECKER_BINARY = "./cpp-check/check"

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Check and possibly submit a solution.")
    parser.add_argument("problem", type=int, help="problem number")
    parser.add_argument("solution", help="solution file")
    args = parser.parse_args()

    #if sys.stdin.isatty():
    #    eprint("waiting for solution on stdin...")

    solfile = args.solution
    probfile = "problems/prob{}".format(args.problem)

    if not(os.path.isfile(CHECKER_BINARY)) :
        eprint("Solution checker {} does not exist. Perhaps you need to call `make`?".format(CHECKER_BINARY))
        exit(1)

    call([CHECKER_BINARY, probfile, solfile])




