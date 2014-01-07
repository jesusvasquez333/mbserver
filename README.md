mbserver
========

INTRODUCTION
------------

Modbus interface for temperature sensors CRYCON18, SI-9300 and SI Twickenham (or any similar). 

This version reads the temperature values from any of the configured instrument at a fixed, defined rate and then writes these value onto local memory that can be access from a remote Modbus/TCP client.

It uses Stéphane Raimbault's <stephane.raimbault@gmail.com> Modbus C library libmodbus. 

Copyright © 2013 Jesus Vasquez <jesusvasquez333@gmail.com>


SOFTWARE DESCRIPTION
--------------------

This software is based on:
1. A client that reads the temperature values from any of the supported thermometers (Cryocon 18, SI-9300, SI Twickenham, or any similar) using their proprietary protocols; and
2. A Modbus-TCP server that shares the temperature reading and some diagnostic information.

This version of the software can read up to 6 thermometers simultaneously, each with 8 temperature channels. Each temperature values is stored as a 32 bits float value (standard IEEE-754), which correspond to 2 Modbus register.

This version have been tested using a Raspberry Pi board (RPi). For connecting the thermometers with RS232 interface, is was used the FTDI USB-COM232-PLUS4 which add four RS232 ports to the RPi. A Schneider PLC was used as a Modbus client to read the temperature values from the RPi.

MODBUS REGISTERS 

The first 96 Modbus registers contain the temperature reading from the 6 instruments (2 register for temperature).

The 97th Modbus register contains information about the communication status with each instrument. The first 6 bits of the LSB indicate is the corresponding thermometer is enable (=1) or disable (=0). On the other hand, the first 6 bits of the MSB indicate if the corresponding thermometer communication status is OK (=0) or not (=1).

		0	thermometer 0 enabled (0 = no; 1 = yes)
		1	thermometer 1 enabled (0 = no; 1 = yes)
	L	2	thermometer 2 enabled (0 = no; 1 = yes)
	S	3	thermometer 3 enabled (0 = no; 1 = yes)
	B	4	thermometer 4 enabled (0 = no; 1 = yes)
		5	thermometer 5 enabled (0 = no; 1 = yes)
		6	- not used -
		7	- not used -
		0	thermometer 0 communication status (0 = ok; 1 = error)
		1	thermometer 1 communication status (0 = ok; 1 = error)
	M	2	thermometer 2 communication status (0 = ok; 1 = error)
	S	3	thermometer 3 communication status (0 = ok; 1 = error)
	B	4	thermometer 4 communication status (0 = ok; 1 = error)
		5	thermometer 5 communication status (0 = ok; 1 = error)
		6	- not used -
		7	- not used -							

The 98th Modbus register is not used

The next 6 Modbus registers indicate the status of the temperature channels for each thermometer. The 99th register contain the status for the thermometer 0, the 100th for the thermometer 1, and so on until the 104th for the thermometer 5. In each register, the 8 bits of the LSB indicate if the corresponding sensor is present (=0) or not (=1), while the 8 bits of the MSB indicate if the corresponding measurement is out of range (=1) or normal (=0).

		0	Sensor 0 is present (0 = yes; 1 = no)
		1	Sensor 1 is present (0 = yes; 1 = no)
	L	2	Sensor 2 is present (0 = yes; 1 = no)
	S	3	Sensor 3 is present (0 = yes; 1 = no)
	B	4	Sensor 4 is present (0 = yes; 1 = no)
		5	Sensor 5 is present (0 = yes; 1 = no)
		6	Sensor 6 is present (0 = yes; 1 = no)
		7	Sensor 7 is present (0 = yes; 1 = no)
		0	Measurement 0 is out of range (0 = no; 1 = yes)
		1	Measurement 1 is out of range (0 = no; 1 = yes)
	M	2	Measurement 2 is out of range (0 = no; 1 = yes)
	S	3	Measurement 3 is out of range (0 = no; 1 = yes)
	B	4	Measurement 4 is out of range (0 = no; 1 = yes)
		5	Measurement 5 is out of range (0 = no; 1 = yes)
		6	Measurement 6 is out of range (0 = no; 1 = yes)
		7	Measurement 7 is out of range (0 = no; 1 = yes)

The starting address for the register table by default is set to 0, but can be modify on the mbserver.h header file. It is possible to modify also the position of the status register as well as the number of thermometer and other parameters. 

CONFIGURATION
-------------

There number, type and local interface of thermometer to be connected to the RPi can be configured using 3 text files located on the /home/pi folder:

1. servver_tcp.txt		(TCP 8 channels thermometer: Cryocon 18)
Add here one line for each thermometer that you want to connect to the RPi, indicating the thermometer IP address and TCP port, separated by and blank space. For example: 192.168.0.100 5000
2. servver_rs232_t1.txt	(RS232 8 channels thermometer: SI 9300)
Add here one line for each thermometer that you want to connect to the RPi, indicating the local serial interface where the thermometer is connected to. For example: /dev/ttyUSB0
3. servver_rs232_t2.txt	(RS232 6 channels thermometer: SI Twickenham, CRYOGENIC)
Add here one line for each thermometer that you want to connect to the RPi, indicating the local serial interface where the thermometer is connected to. For example: /dev/ttyUSB2

COMPILE AND INSTALL
-------------------

You need to install the Stéphane Raimbault's 
<stephane.raimbault@gmail.com> Modbus C library libmodbus first, in order to be able to compile this software. You can find it here: https://github.com/stephane/libmodbus

An example make file is given. With this file you can build with “make” and the install with “make install” (as root). The default installation directory is /usb/bin.

Additionally, a init script example is also given (locate on the template folder) that will be copy to /etc/init.d. With it you can start/stop/add to boot the program with commands like “/etc/init.d/mbserver start”. A log file will be locate at “/home/pi/mbserver_log”.

CONTACT
-------

For any information, feedback, bug report or just to say thanks you can send me an email to jesusvasquez333@gmail.com
