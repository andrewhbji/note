#!/bin/bash
# Program:
# 	This program shows the user's choice
# History:
#2007/04/01 andrea First release
read -p "Please input (Y/N): " yn
#if yn== Y || yn == y echo "Ok, continue"
[ "$yn" == "Y" -o "$yn" == "y" ] && echo "OK, continue" && exit 0
#if yn== N || yn == n echo "Oh, interrupt"
[ "$yn" == "N" -o "$yn" == "n" ] && echo "Oh, interrupt!" && exit 0
echo "I don't know what your choice is" && exit 0
