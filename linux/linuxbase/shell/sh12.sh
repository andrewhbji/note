#!/bin/bash
# Program:
#	This script only accepts the flowing parameter: one, two or three.
# History:
#2007/04/01 andrea First release

echo "This program will print your selection !"
# read -p "Input your choice: " choice # 暂时取消，可以替换！
# case $choice in                      # 暂时取消，可以替换！
case $1 in                             # 现在使用，可以用上面两行替换！
  "one")
	echo "Your choice is ONE"
	;;
  "two")
	echo "Your choice is TWO"
	;;
  "three")
	echo "Your choice is THREE"
	;;
  *)
	echo "Usage $0 {one|two|three}"
	;;
esac
