[TOC]
#shell scripts
##stdio:
- stdin:
read -p "回显内容" 变量
- stdout:
echo -e "回显内容"
>备注：参考 sh01.sh

##日期:
$(date [选项] [格式参数])
>备注：参考 sh02.sh

##算数运算:
total=$($firstnu*$secnu)
>备注：参考 sh03.sh

##script 的运行方式差异 (source, sh script, ./script)
- 直接运行的方式来运行 script, 子shell完成后，子shell的变量就地销毁，不会传回给父shell
- 利用 source 来运行脚本：在父程序中运行，子shell完成后，子shell的变量传回给父shell
>备注：参考 [鸟哥私房菜十三章第二节2.2](http://vbird.dic.ksu.edu.tw/linux_basic/0340bashshell-scripts_2.php#some_ex_run)

##逻辑判断
###test命令
####格式：
test [!] 选项 路径或者字串 [&& 表达式1] [|| 表达式2]
####常用选项
- -文档类型判断
```
-e	#该『档名』是否存在？(常用)
-f	#该『档名』是否存在且为文件(file)？(常用)
-d	#该『档名』是否存在且为目录(directory)？(常用)
-b	#该『档名』是否存在且为一个 block device 装置？
-c	#该『档名』是否存在且为一个 character device 装置？
-S	#该『档名』是否存在且为一个 Socket 文件？
-p	#该『档名』是否存在且为一个 FIFO (pipe) 文件？
-L	#该『档名』是否存在且为一个连结档？
```
- 文档权限判断
```
-r	#侦测该档名是否存在且具有『可读』的权限？
-w	#侦测该档名是否存在且具有『可写』的权限？
-x	#侦测该档名是否存在且具有『可运行』的权限？
-u	#侦测该档名是否存在且具有『SUID』的属性？
-g	#侦测该档名是否存在且具有『SGID』的属性？
-k	#侦测该档名是否存在且具有『Sticky bit』的属性？
-s	#侦测该档名是否存在且为『非空白文件』？
```
- 文件比较
```
-nt	#(newer than)判断 file1 是否比 file2 新
-ot	#(older than)判断 file1 是否比 file2 旧
-ef	#判断 file1 与 file2 是否为同一文件，可用在判断 hard link 的判定上。 主要意义在判定，两个文件是否均指向同一个 inode 哩！
```
-整数比较
```
-eq	#两数值相等 (equal)
-ne	#两数值不等 (not equal)
-gt	n1 #大於 n2 (greater than)
-lt	n1 #小於 n2 (less than)
-ge	n1 #大於等於 n2 (greater than or equal)
-le	n1 #小於等於 n2 (less than or equal)
```
-字串比较
```
test -z string	#判定字串是否为 0 ？若 string 为空字串，则为 true
test -n string	#判定字串是否非为 0 ？若 string 为空字串，则为 false。

test str1 = str2	#判定 str1 是否等於 str2 ，若相等，则回传 true
test str1 != str2	#判定 str1 是否不等於 str2 ，若相等，则回传 false
```
>备注： -n 亦可省略

- 多重条件判定
```
-a	#(and)两状况同时成立！例如 test -r file -a -x file，则 file 同时具有 r 与 x 权限时，才回传 true。
-o	#(or)两状况任何一个成立！例如 test -r file -o -x file，则 file 具有 r 或 x 权限时，就可回传 true。
!	  #反相状态，如 test ! -x file ，当 file 不具有 x 时，回传 true
```
###[]符号判断
>备注：这个[]不是可选项的意思

####格式
[表达式]
>备注：样例参考 sh06.sh, sh06-2.sh

###if else条件判断
>备注：这个[]不是可选项的意思

####格式
```
if [表达式1]; then
 表达式
elif [表达式2]；then
 表达式
else
 表达式
fi
```
>备注：样例参考 sh06-3.sh

###case 条件判断
####格式
```
case  $变量名称 in     #<==关键字为 case ，还有变量前有钱字号
  "第一个变量内容")    #<==每个变量内容建议用双引号括起来，关键字则为小括号 )
	程序段
	;;            #<==每个类别结尾使用两个连续的分号来处理！
  "第二个变量内容")
	程序段
	;;
  *)                  #<==最后一个变量内容都会用 * 来代表所有其他值
	不包含第一个变量内容与第二个变量内容的其他程序运行段
	exit 1
	;;
esac                  #<==最终的 case 结尾！『反过来写』思考一下！
```
>备注：样例参考 sh09-2.sh,sh12.sh

##函数
###函数声明
```
function fname() {
	程序段
}
```
###函数调用
```
fname;
```
>备注：样例参考 sh12-2.sh

###函数传参
```
fname n;
```
>备注：样例参考 sh12-3.sh

##Shell Script传入参数
###参数类型
- $0 shell script本尊
- $1 传入的第一个参数
- $n 传入的第n个参数
- $# 参数个数
- $@ 和 $* 列出所有参数
###参数使用：
获取参数值的时候，必须在“”中使用，如“$0”
>备注：样例参考 sh07.sh，sh12.sh

##shift 偏移传入参数
###格式
shift [n], 默认偏移量为1，n可以指定具体的偏移量
>备注：样例参考 sh08.sh

##循环
###while do done
####格式：
```
while [ condition ] #当condition成立时，执行循环
do
	程序段落
done
```
###until do done
####格式：
```
until [ condition ] #当condition成立时，跳出循环
do
	程序段落
done
```
###for do done 固定循环
####格式
```
for var in con1 con2 con3 ...
do
	程序段
done
```
####功能
类似java 迭代器，第一次回圈时，$var 的内容为 con1，第二次回圈时， $var 的内容为 con2,一次类推
>备注：样例参考 sh15.sh，sh16.sh，sh17.sh , sh18.sh

###for do done 的数值处理
####格式:
```
for (( 初始值; 限制值; 运行步阶 ))
do
	程序段
done
```
####功能
类似c语言for循环
>备注：样例参考 sh19.sh

##Shell Script追踪和debug
###命令格式
```
sh [-nvx] **.sh
```
###常用选项
- -n  ：不要运行 script，仅查询语法的问题；
- -v  ：再运行 sccript 前，先将 scripts 的内容输出到萤幕上；
- -x  ：将使用到的 script 内容显示到萤幕上，这是很有用的参数！
>参考 [鸟哥私房菜十三章第六节](http://vbird.dic.ksu.edu.tw/linux_basic/0340bashshell-scripts_6.php)
