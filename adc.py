from subprocess import Popen, PIPE
import sys
import string
import numpy as np
import matplotlib.pyplot as plt
import random
from time import time
import scipy.signal as signal
import pickle

#stream data by invoking spi program on raspi with
#sudo ./spi | nc 192.168.88.[this machine's local IP] 1025
#this (adc.py) must start first

#read adc output from over nc, bufsize=-1 makes this continuous
process = Popen('nc -l -p 1025',stdout=PIPE,bufsize=-1,shell=True)

iss=[]
x=[]

fs = 400.0  # Sample frequency (Hz)
n_secs=5

count=0
for line in iter(process.stdout.readline,b''):
	val=int(line[2:10],16)
	x.append(val)
	iss.append(count)
 	count+=1
	if count>=n_secs*fs:
		iss.pop(0)
		x.pop(0)
	if (count%400)==199:
		plt.clf()
		plt.plot(iss,x)
		plt.pause(0.000001)


