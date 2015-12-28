#使用者的特殊shell:/sbin/nologin
##什么是nologin
nologin shell用于那些不需要被登陆的系统账号，如用于mail，www服务账号
使用useradd创建账号是用-s选项指定该用户的shell为/sbin/nolongin，这个用户就不能使用shell登陆
##配置nologin提示信息
编辑/etc/nologin.txt即可
```sh
# vi /etc/nologin.txt
This account is system account or mail account.
Please DO NOT use this account to login my Linux server.
```
