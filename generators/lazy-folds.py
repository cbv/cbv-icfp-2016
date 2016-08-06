#!/usr/bin/env python3

import random
import subprocess

f = open('temp.folds', 'w')

def rf():
	den = random.randint(1,1e2)
	num = random.randint(0,den)
	return str(num) + '/' + str(den)

f.write('fold ' + rf() + ',' + rf() + ' ' + rf() + ',' + rf() + '\n')
f.write('fold ' + rf() + ',' + rf() + ' ' + rf() + ',' + rf() + '\n')
f.write('fold ' + rf() + ',' + rf() + ' ' + rf() + ',' + rf() + '\n')
f.write('fold ' + rf() + ',' + rf() + ' ' + rf() + ',' + rf() + '\n')
f.write('fold ' + rf() + ',' + rf() + ' ' + rf() + ',' + rf() + '\n')
f.write('fold ' + rf() + ',' + rf() + ' ' + rf() + ',' + rf() + '\n')
f.write('fold ' + rf() + ',' + rf() + ' ' + rf() + ',' + rf() + '\n')

f.close()

subprocess.call(['../cpp-check/foldup', 'temp.folds', 'temp.solution'])

f = open('temp.solution')
size = len(''.join(f.readlines()))
print("Length: " + str(size))
