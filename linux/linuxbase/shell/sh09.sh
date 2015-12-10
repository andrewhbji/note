#!/bin/bash
# Program:
#	Check $1 is equal to "hello"
# History:
#2007/04/01 andrea First release

if [ "$1" == "hello" ]; then
	echo "Hello, how are you ?"
elif [ "$1" == "" ]; then
	echo "You MUST input parameters, ex> {$0 someword}"
else
	echo "The only parameter is 'hello', ex> {$0 hello}"
fi
