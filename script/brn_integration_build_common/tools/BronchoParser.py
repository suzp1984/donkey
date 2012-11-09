#!/usr/bin/python
import ConfigParser
from optparse import OptionParser
import os, sys


option_parser = OptionParser()
option_parser.add_option("-l", "--list", help="list all available devices", action="store_true", dest="get_list")
option_parser.add_option("-g", "--get_opt", help="get the option items, usage: [device] [item]", action="store_true", dest="get_item")
option_parser.add_option("-j", "--judge_section", help="judge: [item]", action="store_true", dest="judge_section")
option_parser.add_option("-d", "--get_devices_list", help="get the devices list", action="store_true", dest="get_devices_list")
option_parser.add_option("-m", "--get_modules_list", help="get the modules list", action="store_true", dest="get_modules_list")
option_parser.add_option("-f", "--filename", help="get config file", action="store", dest="config_file")
option_parser.set_defaults(get_list=False, get_item=False, get_devices_list=False, get_modules_list=False, config_file="../config")

(option_cmd, args) = option_parser.parse_args()

config_parser = ConfigParser.ConfigParser()
config_parser.read(option_cmd.config_file)

sections = config_parser.sections()
sections.remove("default")

def main():
	if(option_cmd.get_list):
		print "  [The available devices] "+"\n"
		for item in sections: 
			print "\t"+item
		return

	if(option_cmd.judge_section):
		if args[0] in sections:
			print 'true'
		else:
			print 'false'

	if(option_cmd.get_item):
		print config_parser.get(args[0], args[1])
		return

		
	if(option_cmd.get_devices_list):
		print config_parser.get("default", "devices")
		return

	if(option_cmd.get_modules_list):
		print config_parser.get("default", "modules")
		return


if __name__ == "__main__":
	main()
