#!/usr/bin/env python3

import paho.mqtt.client as mqtt

# This is the Publisher
# really slow! after the publish. that's why it need loop_forever
# def on_connect(client, userdata, result):
#	print("connect with result "+str(result))

def on_publish(client, userdata, mid):
	print("published")

client = mqtt.Client()
client.on_publish = on_publish
client.connect("localhost",1883,60)
client.publish("topic/test", "Hello world!");
client.disconnect();