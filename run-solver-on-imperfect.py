#!/usr/bin/env python3

import argparse
import subprocess
import os
import time
import re
import signal

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Run a solver on every problem that hasn't been perfectly solved.")
	parser.add_argument("solver", help="solver binary")
	parser.add_argument("--threads", help="threads to spawn", type=int, default=1)
	parser.add_argument("--timeout", help="solver timeout (0 == unlimited)", type=int, default=0)
	parser.add_argument("--min", help="minimum problem number", type=int, default=0)
	parser.add_argument("--max", help="maximum problem number", type=int, default=999999)

	args = parser.parse_args()

	problem_files = []
	for root, dirs, files in os.walk('problems'):
		for f in files:
			if re.match(r'prob\d+',f):
				problem_files.append(f)
		break
	print("Have " + str(len(problem_files)) + " problems.")

	todo = []
	for problem_file in problem_files:
		number = int(problem_file[4:])
		if number < args.min or number > args.max:
			continue
		db_dir = 'reptiloid-db/problem{0:06d}'.format(number)
		done = False
		try:
			best = os.readlink(db_dir + '/best_submitted')
			if best.startswith('solution_1.0000000'):
				done = True
		except:
			pass
		if not done:
			todo.append(number)
	print("Of which " + str(len(todo)) + " are not perfectly solved.")

	#TODO: could sort in some order, I guess

	pending = []

	def killall(signum, frame):
		global pending
		print("--- killing all solvers ---")
		for p in pending:
			try:
				proc.kill()
			except:
				pass
		exit(1)
		
	signal.signal(signal.SIGINT, killall)

	while len(todo) > 0 or len(pending) > 0:
		still_pending = []
		for p in pending:
			number = p[0]
			soln_file = p[1]
			proc = p[2]
			elapsed = p[3] + 1

			if proc.poll() == None:
				if args.timeout != 0 and elapsed > args.timeout:
					print("Killing " + " ".join(proc.args))
					proc.kill()
				else:
					still_pending.append( (number, soln_file, proc, elapsed) )
			else:
				if proc.returncode != 0:
					print("Got error running: '" + " ".join(proc.args) + "'")
					if os.path.exists(soln_file):
						print("  (will try to submit anyway)")
						#TODO
				if os.path.exists(soln_file):
					print("Submitting " + soln_file)
					subprocess.call(['./reptiloid.py', str(number), soln_file])
					time.sleep(1) #rate limit
					os.unlink(soln_file)
				else:
					print("NO SOLUTION (" + soln_file + ")")
		pending = still_pending
		while len(pending) < args.threads and len(todo) > 0:
			number = todo.pop(0)
			soln_file = "TMP-soln-{}.txt".format(number)
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