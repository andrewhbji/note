#!/bin/bash
# ex18.sh
# 在下面三个可选的服务器中进行 whois 域名查询:
# ripe.net, cw.net, radb.net
# 将这个脚本重命名为 'wh' 后放在 /usr/local/bin 目录下
# 这个脚本需要进行符号链接:
# ln -s /usr/local/bin/wh /usr/local/bin/wh-ripe
# ln -s /usr/local/bin/wh /usr/local/bin/wh-apnic
# ln -s /usr/local/bin/wh /usr/local/bin/wh-tucows
E_NOARGS=75
if [ -z "$1" ]
then
  echo "Usage: `basename $0` [domain-name]"
  exit $E_NOARGS
fi
# 检查脚本名,然后调用相对应的服务器进行查询。
case `basename $0` in    # 你也可以写成:    case ${0##*/} in
    "wh"       ) whois $1@whois.tucows.com;;
    "wh-ripe"  ) whois $1@whois.ripe.net;;
    "wh-apnic" ) whois $1@whois.apnic.net;;
    "wh-cw"    ) whois $1@whois.cw.net;;
    *          ) echo "Usage: `basename $0` [domain-name]";;
esac
exit $?
