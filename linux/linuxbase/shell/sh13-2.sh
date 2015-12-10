#!/bin/bash
# Program:
#	Repeat question until user input correct answer.
# History:
#2007/04/01 andrea First release

until [ "$yn" == "yes" -o "$yn" == "YES" ]
do
	read -p "Please input yes/YES to stop this program: " yn
done
echo "OK! you input the correct answer."
