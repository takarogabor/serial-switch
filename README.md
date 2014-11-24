serial-switch
=============

TTL Serial port on/off switch with PIC16F628A

PIC receive commands at UART serial port and turn a device on/off on RB5 output pin.
RB4 is a heartbeat output with a LED.

Protocol:  
 * Turn off:  
command: ki  
response: ok
 * Turn on:  
command: be  
response: ok  
 * Get status:  
command: st  
response: ki|be  

Every command and response closed by CR+LF.

Sample circuit: 
![Sample circuit](/serial_switch_circuit.jpg)
