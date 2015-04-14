HL-ATCMD
============

This simple command line tool for sending AT command to a serial device.


Build
-----
Just use the Makefile:
*make*

to build the executable.

Run
---
HLATCMD can be launched as follow::
~~~
./hl-atcmd -d device -b baudrate -f filter at-cmd
~~~

Arguments:
* -d:		specifies the device port. If ommited the default device port is /dev/ttyUSB0
* -b: 		specifies the communication baudrate. If ommited the default baud rate is 115200 bauds
* -f:		optional argument specifying the filter string used to extract result from the terminal's response
* at-cmd:	mandatory, at-cmd is the AT commmand to be sent to the device specified by -d argument.
