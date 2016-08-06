#!/usr/bin/env python3

import random
import subprocess
import os

f = open('temp.folds', 'w')

def rf():
	den = random.randint(1,10)
	num = random.randint(0,den)
	return str(num) + '/' + str(den)

f.write('fold 0,1/2 1,1/2\n')
f.write('fold 0,1/4 1,13/24\n')

for x in range(0,random.randint(1,10)):
	f.write('fold ' + rf() + ',' + rf() + ' ' + rf() + ',' + rf() + '\n')

f.close()

subprocess.call(['../cpp-check/foldup', 'temp.folds', 'temp.solution'])

f = open('temp.solution')
size = len(''.join(f.readlines()))
f.close()
print("Length: " + str(size))

if size > 2500:
	print('wooooaaaahhh that\'s too big')
	os.unlink('temp.folds')
	os.unlink('temp.solution')
else:
	subprocess.call(['../cpp-check/show', 'temp.solution'])
