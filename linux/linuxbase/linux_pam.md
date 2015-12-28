[TOC]
#PAM (Pluggable Authentication Modules)嵌入式认证模块
##什么是PAM
PAM为Linux系统以及httpd、Postfix(email)等服务提供了统一的认证机制，并提供一套API供Linux系统、httpd、Postfix使用
##PAM配置
###程序呼叫PAM的流程
1. 用户开始运行 /usr/bin/passwd 这支程序，并输入口令；
2. passwd 呼叫 PAM 模块进行验证；
3. PAM 模块会到 /etc/pam.d/ 找寻与程序 (passwd) 同名的配置文件；
4. 依据 /etc/pam.d/passwd 内的配置，引用相关的 PAM 模块逐步进行验证分析；
5. 将验证结果 (成功、失败以及其他信息) 回传给 passwd 这支程序；
6. passwd 这支程序会根据 PAM 回传的结果决定下一个动作 (重新输入新口令或者通过验证！)
###PAM配置文件
位于/etc/pam.d下，文件格式如 runuser-l
```sh
$ cat runuser-l
#%PAM-1.0
auth		include		runuser
session		optional	pam_keyinit.so force revoke
-session	optional	pam_systemd.so
session		include		runuser
```
>备注：include代表请调用runuser文件的意思

####字段
#####验证类别字段
- auth： 通过passwd来检验使用者身份
- account： 执行授权，检验使用者是否由正确的权限
- session： 管理账号登陆期间ＰＡＭ所给予的环境配置，并记录用户登陆与注销是的信息
- password:　提供验证的修订工作，如修改／变更passwd

>备注：　这四个验证类型通常是有顺序的

#####验证的控制标志
- required： 此验证若成功则带有 success (成功) 的标志，若失败则带有 failure的标志，但不论成功或失败都会继续后续的验证流程
- requisite：若验证失败则立刻回报原程序 failure的标志，并终止后续的验证流程。若验证成功则带有 success的标志并继续后续的验证流程
- sufficient：若验证成功则立刻回传 success给原程序，并终止后续的验证流程；若验证失败则带有 failure标志并继续后续的验证流程
- optional：这个模块控件目大多是在显示信息而已，并不是用在验证方面的
##常用模块简介
###PAM相关路径
- /etc/pam.d/*：每个程序个别的 PAM 配置文件；
- /lib/security/*：PAM 模块文件的实际放置目录；
- /etc/security/*：其他 PAM 环境的配置文件；
- /usr/share/doc/pam-*/：详细的 PAM 说明文件。
###常用模块
- pam_securetty.so: 限制系统管理员 (root) 只能够从安全的 (secure) 终端机登陆
- pam_nologin.so: 这个模块可以限制一般用户是否能够登陆主机之用
- pam_selinux.so:
- pam_console.so: 当系统出现某些问题，或者是某些时刻你需要使用特殊的终端接口 (例如 RS232 之类的终端联机设备) 登陆主机时，这个模块可以帮助处理一些文件权限的问题，让使用者可以透过特殊终端接口(console) 顺利的登陆系统
- pam_loginuid.so: 我们知道系统账号与一般账号的 UID 是不同的！一般账号 UID 均大于 500 才合理。 因此，为了验证使用者的 UID 真的是我们所需要的数值，可以使用这个模块来进行规范
- pam_env.so: 用来配置环境变量的一个模块，如果你有需要额外的环境变量配置，可以参考/etc/security/pam_env.conf 这个文件的详细说明
- pam_unix.so: 这个太重要了，这个模块可以用在验证阶段的认证功能，可以用在授权阶段的账号许可证管理， 可以用在会议阶段的登录文件记录等，甚至也可以用在口令升级阶段的检验
- pam_cracklib.so： 可以用来检验口令的强度
- pam_limits.so： 和ulimit相关
##login在PAM验证机制中的流程
1. 验证阶段 (auth)：首先，(a)会先经过 pam_securetty.so 判断，如果使用者是 root 时，则会参考 /etc/securetty 的配置； 接下来(b)经过 pam_env.so 配置额外的环境变量；再(c)透过 pam_unix.so检验口令，若通过则回报 login 程序；若不通过则(d)继续往下以 pam_succeed_if.so 判断 UID 是否大于 500 ，若小于 500则回报失败，否则再往下 (e)以 pam_deny.so 拒绝联机。
2. 授权阶段 (account)：(a)先以 pam_nologin.so 判断 /etc/nologin 是否存在，若存在则不许一般使用者登陆； (b)接下来以 pam_unix 进行账号管理，再以 (c) pam_succeed_if.so 判断 UID是否小于 500 ，若小于 500 则不记录登录信息。(d)最后以 pam_permit.so 允许该账号登陆。
3. 口令阶段 (password)：(a)先以 pam_cracklib.so 配置口令仅能尝试错误 3 次；(b)接下来以 pam_unix.so 透过 md5, shadow 等功能进行口令检验，若通过则回报 login 程序，若不通过则 (c)以pam_deny.so 拒绝登陆。
4. 会议阶段 (session)：(a)先以 pam_selinux.so 暂时关闭 SELinux；(b)使用 pam_limits.so 配置好用户能够操作的系统资源； (c)登陆成功后开始记录相关信息在登录文件中； (d)以 pam_loginuid.so规范不同的 UID 权限；(e)开启 pam_selinux.so 的功能。
##其他文件
###/etc/security/limits.conf
系统管理员通过PAM来统一管理ulimit功能的配置
###/var/log/secure, /var/log/messages
日志，记录无法登陆或者其他无法预期的错误，如多重登陆错误
