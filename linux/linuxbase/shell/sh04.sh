#!/bin/bash
# Program:
#	User inputs 2 integer numbers; program will cross these two numbers.
# History:
#2007/04/01 andrea First release
echo -e "You SHOULD input 2 numbers, I will cross them! \n"
read -p "first number:  " firstnu
read -p "second number: " secnu
total=$(($firstnu*$secnu))
echo -e "\nThe result of $firstnu x $secnu is ==> $total"
