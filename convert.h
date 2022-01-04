#ifndef _CONVERT_H
#define _CONVERT_H

#define CONVERT_INVALID_PORT -1

// convert takes a port name, such as PC13 and converts it into the
// corresponding GPIO number for the Pine A64+
// Returns CONVERT_INVALID_PORT if port is not a valid name
// References:
// * http://forum.pine64.org/showthread.php?tid=474
// * https://gist.github.com/longsleep/6ab75289bf92cbe3d02a0c5d3f6a4764
int convert(char *port);

#endif // _CONVERT_H
