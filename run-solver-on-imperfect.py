#!/usr/bin/env python3

import argparse
import subprocess
import os
import time
import re
import signal
import random

def is_done(number):
	db_dir = 'reptiloid-db/problem{0:06d}'.format(number)
	if os.path.isfile(os.path.join(db_dir, "THIS_PROBLEM_IS_OURS")):
		return True
	done = False
	try:
		best = os.readlink(db_dir + '/best_submitted')
		if best.startswith('solution_1.0000000'):
			return True
	except:
		pass
	return False


if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Run a solver on every problem that hasn't been perfectly solved.")
	parser.add_argument("solver", help="solver binary")
	parser.add_argument("--threads", help="threads to spawn", type=int, default=1)
	parser.add_argument("--timeout", help="solver timeout (0 == unlimited)", type=int, default=0)
	parser.add_argument("--min", help="minimum problem number", type=int, default=0)
	parser.add_argument("--max", help="maximum problem number", type=int, default=999999)
	parser.add_argument("--include", help="list of problems to include", type=str, default="")
	parser.add_argument("--exclude", help="list of problems to exclude", type=str, default="")

	parser.add_argument("--log-timeouts", help="file to store timeout problem numbers", type=str, default='')
	parser.add_argument("--log-errors", help="file to store solver error problem numbers", type=str, default='')



	parser.add_argument("--normalize", help="call the named utility on each solution before submitting", type=str, default='')

	parser.add_argument("--random", help="should the problems be shuffled?", dest='random', action='store_true')
	parser.set_defaults(random=False)

	parser.add_argument("--reverse", help="should the problems be reversed?", dest='reverse', action='store_true')
	parser.set_defaults(reverse=False)

	parser.add_argument("--no-cleanup", help="delete temporary files", dest="cleanup", action='store_false')
	parser.set_defaults(cleanup=True)

	parser.add_argument("--test", help="definitely do this problem number", type=int, default=0)

	args = parser.parse_args()

	timeout_file = None
	if args.log_timeouts != '':
		timeout_file = open(args.log_timeouts, 'w')

	error_file = None
	if args.log_errors != '':
		error_file = open(args.log_errors, 'w')

	problem_files = []
	for root, dirs, files in os.walk('problems'):
		for f in files:
			if re.match(r'prob\d+',f):
				problem_files.append(f)
		break
	print("Have " + str(len(problem_files)) + " problems.")

	include = None
	if args.include:
		include = set()
		with open(args.include, 'r') as f:
			for n in re.split(r'\s+', " ".join(f.readlines())):
				try:
					include.add(int(n))
				except:
					pass #worst file handling ever
		print("Specifically including " + str(len(include)) + " files.")

	exclude = None
	if args.exclude:
		exclude = set()
		with open(args.exclude, 'r') as f:
			for n in re.split(r'\s+', " ".join(f.readlines())):
				try:
					exclude.add(int(n))
				except:
					pass #worst file handling ever
		print("Specifically excluding " + str(len(exclude)) + " files.")

				


	todo = []
	for problem_file in problem_files:
		number = int(problem_file[4:])
		if number < args.min or number > args.max:
			continue
		if include is not None and number not in include:
			continue
		if exclude is not None and number in exclude:
			continue
		if not is_done(number):
			todo.append(number)
	print("Of which " + str(len(todo)) + " are not perfectly solved.")

	#TODO: could sort in some order, I guess
	todo.sort()
	if args.random:
		random.shuffle(todo)
	if args.reverse:
		todo.reverse()

	if args.test != 0:
		todo = [args.test]

	pending = []

	def submit(soln_file):
		print("Submitting " + soln_file)
		subprocess.call(['./reptiloid.py', str(number), soln_file])
		if args.normalize != '':
			time.sleep(1) #rate limit
			print("Normalizing and re-submitting " + soln_file)
			norm_file = soln_file + '-norm'
			subprocess.call([args.normalize, soln_file, norm_file])
			subprocess.call(['./reptiloid.py', str(number), norm_file])
			time.sleep(1) #rate limit
			if args.cleanup:
				try:
					os.unlink(norm_file)
				except:
					pass
		if args.cleanup:
			try:
				os.unlink(soln_file)
			except:
				pass


	def killall(signum, frame):
		global pending
		print("--- killing all solvers ---")
		for p in pending:
			try:
				proc.kill()
				proc.send_signal(9)
			except:
				pass
		exit(1)
		
	signal.signal(signal.SIGINT, killall)

	while len(todo) > 0 or len(pending) > 0:
		print(str(len(todo)) + " remain...")
		still_pending = []
		for p in pending:
			number = p[0]
			soln_file = p[1]
			proc = p[2]
			elapsed = p[3] + 1

			if proc.poll() == None:
				if args.timeout != 0 and elapsed > args.timeout:
					if timeout_file is not None:
						timeout_file.write(str(number) + '\n')
						timeout_file.flush()
					print("Killing " + " ".join(proc.args))
					proc.kill()
					proc.send_signal(9)
					if os.path.exists(soln_file):
						submit(soln_file)
				else:
					still_pending.append( (number, soln_file, proc, elapsed) )
			else:
				if proc.returncode != 0:
					print("Got error running: '" + " ".join(proc.args) + "'")
					if error_file is not None:
						error_file.write(str(number) + '\n')
						error_file.flush()
					if os.path.exists(soln_file):
						print("  (will try to submit anyway)")
				if os.path.exists(soln_file):
					submit(soln_file)
				else:
					print("NO SOLUTION (" + soln_file + ")")
		pending = still_pending
		while len(pending) < args.threads and len(todo) > 0:
			number = todo.pop(0)
			if number != args.test and is_done(number): continue #check again just in case

			soln_file = "TMP-soln-{}.txt".format(number)
			try:
				os.unlink(soln_file)
			except:
				pass
			cmd = [args.solver, 'problems/prob{}'.format(number), soln_file]
			print("Launching '" + " ".join(cmd) + "'")
			proc = subprocess.Popen([args.solver, 'problems/prob{}'.format(number), soln_file])
			pending.append( (number, soln_file, proc, 0) )
		time.sleep(1)
	

	#problem_number = args.problem_number
#    probfile = "problems/prob{}".format(problem_number)

 #   TMP = "sol.txt"
  #  if subprocess.call([args.solver, probfile, TMP]) != 0:
   #     print("error running solver")
	#    exit(1)

#    subprocess.call(["./reptiloid.py", "{}".format(problem_number), TMP])
