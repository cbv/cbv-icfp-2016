#!/usr/bin/env python3

import argparse
import subprocess


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Check and possibly submit a solution.")
    parser.add_argument("solver", help="solver binary")
    parser.add_argument("problem_number", type=int, help="problem number")

    args = parser.parse_args()

    problem_number = args.problem_number
    probfile = "problems/prob{}".format(problem_number)

    TMP = "sol.txt"
    if subprocess.call([args.solver, probfile, TMP]) != 0:
        print("error running solver")
        exit(1)

    subprocess.call(["./reptiloid.py", "{}".format(problem_number), TMP])
