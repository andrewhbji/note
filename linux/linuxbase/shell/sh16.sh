#!/bin/bash
# Program
#       Use id, finger command to check system account's information.
# History
#2007/04/01 andrea First release

users=$(cut -d ':' -f1 /etc/passwd)  # 撷取帐号名称
for username in $users               # 开始回圈进行！
do
        id $username
        finger $username
done

