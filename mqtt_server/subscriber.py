#!/usr/bin/env python3
import paho.mqtt.subscribe as subscribe
import os
import atexit

filename = "tmp_local_connector/local_log_" # common part of each log file
myfilename = ""
logfile = open("tmp_local_connector/total_log.txt", "w") # total log file, copy the log in terminal to this file

# define a function to be executed before exit
def clearWonderShaper():
	os.system("sudo wondershaper clear enp2s0") # clear bw settings, very important!
	logfile.close()
	print("clean wondershaper and close logfile")
atexit.register(clearWonderShaper) # register exit handlers

# print the msg to terminal and write to logfile
def my_print(s):
	print(s)
	logfile.write(s+"\r\n")

# call back when receive a subcribe
def on_message(client, userdata, message):
	global myfilename # need to make it global!
	if message.topic == "house.hand":
		my_print("%s" % message.topic)
	elif message.topic == "tmp":
		my_print("%s" % (message.payload))
		my_print("write to %s" % myfilename)
		fw = open(myfilename, "a")
		fw.write(message.payload)
		fw.close()
	elif message.topic == "parameter":
		para = message.payload.strip().split(' ');
		my_print("%s" % (message.payload)) # bw, sample_rate
		os.system("sudo wondershaper enp2s0 %s %s" % (para[0], para[0])) # open bw control
		# os.system("sudo wondershaper enp2s0")
		myfilename = filename + ("%s_%s.txt" % (para[0], para[1])) # specify filename
		open(myfilename, "w").close() # clear file
	elif message.topic == "sync":
		my_print("%s %s" % (message.topic, message.payload))
	
# this is a block function
# Definition: callback(callback, topics, qos=0, userdata=None, hostname="localhost",
#    port=1883, client_id="", keepalive=60, will=None, auth=None, tls=None,
#    protocol=mqtt.MQTTv311)
subscribe.callback(on_message, ["house.hand", "tmp", "parameter", "sync"], qos=2, \
	hostname="137.110.160.230", port=1883, client_id="linux", keepalive=60, \
	auth={'username':"xiaofan",'password':"0808"})