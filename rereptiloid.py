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


for root, dirs, files in os.walk('problems'):
	for f in files:
		if not re.match(r'prob\d+',f):
			continue
		number = int(f[4:])
		if is_done(number):
			continue
		db_dir = 'reptiloid-db/problem{0:06d}'.format(number)
		try:
			l = os.listdir(db_dir)
		except:
			l = []
		for s in l:
			if s.startswith('solution_1.0000000'):
				path = db_dir + '/' + s
				print("Trying " + path)
				subprocess.call(['./reptiloid.py', str(number), path])
				time.sleep(1 + random.random())

