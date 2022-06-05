## dht-gpio

Experimental code for using the GPIO on a Pine A64+ arm board.

`readtemp` is a program for reading temperature and relative humidity from a DHT
sensor connected to the GPIO of a Pine A64+ board.

`blink` is a test program to blink a led connected to the GPIO.

`converter` is a helper program to figure out the GPIO port configuration.

`rtlog` is a helper program to figure out what kind of expectations to have on
the "very soft realtime" characteristics of the system.

## Dependencies

The gpiod library
    
    apt install gpiod libgpiod-dev libgpiod-doc

Reference/example on soft real-time priority in linux: <http://www.isy.liu.se/edu/kurs/TSEA81/lecture_linux_realtime.html>
