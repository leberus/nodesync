#!/bin/bash

TEST_ENV="/home/os/lab/bin/"
BIN="nodesync"
	
	cd ..
	make
	if [ $? -eq 0 ]
	then
		mv $BIN $TEST_ENV
		make clean
		echo "Ready to fire!"
		ret=0
	else
		echo "Error compiling!"
		ret=1
	fi

	exit $ret
