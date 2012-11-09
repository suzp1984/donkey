#!/usr/bin/python
import os,sys
from optparse import OptionParser

#r=raw_input("type a directory name:")
option_parser = OptionParser()

(option_cmd, args) = option_parser.parse_args()

def main():
	for root,dirs,files in os.walk(args[0]):
		for f in files:
			print os.path.join(root,f)


def test():
	print args[0]

if __name__ == "__main__":
	main();
