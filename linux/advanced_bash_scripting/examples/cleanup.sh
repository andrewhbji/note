#	Cleanup version 1
#	使用root权限运行
cd	/var/log
cat	/dev/null	>	messages
cat	/dev/null	>	wtmp
echo	"Log	files	cleaned	up."
