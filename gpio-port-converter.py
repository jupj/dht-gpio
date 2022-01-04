#!/usr/bin/python3

# Source: https://gist.github.com/longsleep/6ab75289bf92cbe3d02a0c5d3f6a4764

import sys
import string

def convert(value):
	value = value.upper()
	alp = value[1]
	idx = string.ascii_uppercase.index(alp)
	num = int(value[2:], 10)
	res = idx * 32 + num
	return res

if __name__ == "__main__":
	args = sys.argv[1:]
	if not args:
		print("Usage: %s <pin>" % sys.argv[0])
		sys.exit(1)

	print("%d" % convert(args[0]))
