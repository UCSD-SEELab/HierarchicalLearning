#############################################################
# Project: Basic processing for abnormality detection
# Author: Xiaofan Yu
# Date: 11/15/2018
#############################################################
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import statistics as stat
import math
from scipy.optimize import curve_fit 

filename = 'MQTT_Messages_Rishi_11-13-18.txt'
topic_list = ['"temperature"', '"humidity"', '"pm2_5"', '"pm1"', '"pm10"']
device_list = ['AirBeam-8042']

#
# read_topic - read values of specific topic comming from specific device
# Parameter - topic - list of strings of the topic
#			  devicename - list of strings of the device name 
# Return - data - dictionary of data for various topics
#
def read_topic(topic, device):
	data = {}
	with open(filename, 'r') as f:
		for line in f:
			for this_dev in device:
				dev = line.find(this_dev)
				if dev == -1: # this device does not exist in this line
					continue
				# this device do exist in this line
				for this_topic in topic: # iterature the topic list
					pos = line.find(this_topic)
					if pos == -1: # if this topic does not exist in this line
						continue
					# extract the value for this topic
					sub_line = line[pos:]
					# print(sub_line)
					sub_line = sub_line.split(':', 2)[1].strip()
					this_value = sub_line.split(' ', 2)[0].strip(' ,}')
					# print(this_topic, this_value)
					# add it to data dictionary
					if (data.has_key(this_topic)):
						data[this_topic].append(float(this_value))
					else:
						data[this_topic] = []
						data[this_topic].append(float(this_value))
	return data

#
# print_graph - print univariate time and value distribution,
#				and the correlation between any two topics
# Parameter - data - data in different topic, a dictionary
#			  topic - list of topics
#
def print_graph(data, topic):
	cnt = 1 # cnt for graphs
	# print individual time and value distribution
	for this_topic in topic:
		if data.has_key(this_topic):
			# draw time distribution
			plt.figure()
			plt.plot(data[this_topic], 'b-')
			plt.xlabel('time step')
			plt.ylabel(this_topic)
			fig_name = './img/' + this_topic + '_time.png'
			plt.savefig(fig_name, dpi=300)
			plt.close()
			# draw data distribution
			plt.figure()
			sns.distplot(data[this_topic], rug=True)
			plt.xlabel(this_topic)
			plt.ylabel('frequency')
			fig_name = './img/' + this_topic + '_dist.png'
			plt.savefig(fig_name, dpi=300)
			plt.close()
	# print 2-D distribution over two topics
	for id1 in range(len(topic)):
		for id2 in range(id1+1, len(topic)):
			if data.has_key(topic[id1]) and data.has_key(topic[id2]):
				plt.figure()
				plt.plot(data[topic[id1]], data[topic[id2]], 'bo')
				plt.xlabel(topic[id1])
				plt.ylabel(topic[id2])
				fig_name = './img/' + topic[id1] + '_' + topic[id2] + '.png'
				plt.savefig(fig_name, dpi=300)
				plt.close()

#
# save_sorted_data - save sorted data according to topics
# Parameter - data - data in different topic, a dictionary
#			  topic - list of topics
#
def save_sorted_data(data, topic):
	for this_topic in topic:
		data[this_topic].sort()
	savefile = 'res_' + filename
	with open(savefile, 'w') as fw:
		fw.write(str(data))

#
# stat_cal - calculate the statitical parameter: mean, std variance, variance
# Parameter - data - data in different topic, a dictionary
#			  topic - list of topics
#
def stat_cal(data, topic):
	stat_para = {} # to store statistic parameters of various topics
	for this_topic in topic:
		stat_para[this_topic] = []
		stat_para[this_topic].append(stat.mean(data[this_topic])) # append mean
		stat_para[this_topic].append(stat.stdev(data[this_topic])) # append sample stdv
		# stat_para[this_topic].append(stat.variance(data[this_topic])) # append sample variance
	print("statistical parameters, mean and std variance")
	print(stat_para)
	return stat_para

#
# gaussian function
#
def gaussian(x, *p):
	amp, cen, var, d = p
	y = amp*np.exp(-np.power((x - cen), 2.)/(2. * var**2.)) + d

	return y

#
# gaussian_fit - try to fit the data with gaussian
# Parameter - data - data in different topic, a dictionary
#			  topic - list of topics
#			  stat_para - statistical parameters, useful in initial value setting
#
def gaussian_fit(data, topic, stat_para):
	gaussian_para = {}
	for this_topic in topic:
		freq = {} # init an empty dict to store frequency
		for value in data[this_topic]: # iterate all the data in this topic
			if freq.has_key(value):
				freq[value] += 1.0
			else:
				freq[value] = 1.0 # start from 1
		length = len(data[this_topic]) # total len of all the data in this topic
		for value in freq.keys():
			freq[value] /= length # normalize

		x = freq.keys()
		y = [freq[x_] for x_ in x]

		p_initial = [1.0, stat_para[this_topic][0], stat_para[this_topic][1], 0.0] # set initial value
		try: # try to fit the frequency distribution with gaussian
			best_vals, covar = curve_fit(gaussian, x, y, p0=p_initial)
		except Exception as e:
			print(this_topic, e)
		else: # if success, print and save parameters
			# save mean and std variance as parameters
			gaussian_para[this_topic] = [best_vals[1], best_vals[2]]
			y_fit = gaussian(x, *best_vals)
			plt.figure()
			data_point, = plt.plot(x, y, 'bo')
			fitted_line, = plt.plot(x, y_fit, 'r-')
			plt.legend([data_point, fitted_line], ['data', 'fitted_line'])
			plt.xlabel(this_topic)
			plt.ylabel("frequency")
			fig_name = './img/' + this_topic + '_gaussian.png'
			plt.savefig(fig_name, dpi=300)
			plt.close()
			
	print("gaussian-fitted parameters, mean and std variance")
	print(gaussian_para)
	return gaussian_para

#
# abnormal_detection - judge whether the data is normal
# Parameter - value - one new value obtained
#			  topic - the topic of this new value
#			  para - mean and std variance used in judging
#			  k - the coefficient times with std variance
# Return - 0 - normal, 1 - abnormal
# We perceive the data in mean-k*stdvar ~ mean+k*stdvar as normal ones
#
def abnormal_detect(value, topic, para, k):
	if para.has_key(topic):
		if value < para[topic][0] - k * para[topic][1]:
			print(topic, value, "data smaller than ", para[topic][0] - k * para[topic][1])
			return 1
		if value > para[topic][0] + k * para[topic][1]:
			print(topic, value, "data bigger than ", para[topic][0] + k * para[topic][1])
			return 1
		print(topic, value, "data is normal")
		return 0
	else:
		print(topic, "is not available in this parameter setting")


data = read_topic(topic_list, device_list)
print_graph(data, topic_list) # print and save the distribution graph
# print(data)
# save_sorted_data(data, topic_list)
stat_para = stat_cal(data, topic_list)
gaussian_para = gaussian_fit(data, topic_list, stat_para)
# several tests
print("Examples of independent local abnormal detection")
print(abnormal_detect(10.0, '"pm10"', gaussian_para, 1.0))
print(abnormal_detect(34.0, '"temperature"', gaussian_para, 1.0))
print(abnormal_detect(34.0, '"temperature"', stat_para, 1.0))
print(abnormal_detect(50.0, '"humidity"', stat_para, 1.0))
