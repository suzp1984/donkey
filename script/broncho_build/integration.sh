#!/bin/bash
#

if [ "$0" = "./integration/integration.sh" -o -d ./integration ]
then
	cd integration
fi

# svn part
if [ -d ./broncho_build ]; then
	echo -e "alreay have broncho_build script, if you want update, do it your self \n"
else
	svn co svn+ssh://svn@192.168.1.168/all/broncho_build/
fi

cd ./broncho_build &&  sh integration.sh "$1" "$2" "$3" "$4" "$5" "$6"
