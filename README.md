# Wifi-Water-Flood-Sensor
Wifi Water/Flood Sensor with ESP-01

Use ESP-01 to provide Wifi and MQTT client to detect presence of water, and optionally use directly connected speaker/buzzer to provide localized alarm.

My specific application for project is to detect flood/water inside my sump pump well, in the case of sump pump failure.  When water is detected by 2 open wires, it would send message to MQTT broker.  MQTT broker would then relays the message to NodeRED.  Upon receiving MQTT message, NodeRED would send announcement to multiple google home devices and also optionally send message to cellphone/browser via pushbullet

I have a passion for DIY home automation, and slowly adding IOT to my home. Google home has been one of the central function in my home automation. 
