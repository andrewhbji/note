[TOC]
#文件格式化
##printf 格式化打印
###格式
printf '打印格式' 内容
###样例
```
printf '%s\t %s\t %s\t %s\t %s\t \n' $(cat printf.txt)
```
###常用格式符：
- \a    ：警告声音输出
- \b     ：倒退键(backspace)
- \f     ：清除萤幕 (form feed)
- \n     ：输出新的一行
- \r     ：亦即 Enter 按键
- \t     ：水平的 [tab] 按键
- \v     ：垂直的 [tab] 按键
- \xNN   ：NN 为两位数的数字，可以转换数字成为字节。
- %ns    ：那个 n 是数字， s 代表 string ，亦即多少个字节；
- %ni    ：那个 n 是数字， i 代表 integer ，亦即多少整数码数；
- %N.nf  ：那个 n 与 N 都是数字， f 代表 floating (浮点)，如果有小数码数，假设我共要十个位数，但小数点有两位，即为 %10.2f 罗！

##awk中使用printf
awk是一个非常棒的可编程的数据处理工具，支持独立的awk语言
###格式
```
awk ‘数据处理逻辑’ 文件名
```
###样例
```
awk '{print $1 "\t" $3}' #打印第一列和第三列
awk '{print $1 "\t lines: " NR "\t columns: " NF}' #打印当前行号以及该行列数
awk 'BEGIN {FS=":"} $3 < 10 {print $1 "\t " $3}' #BEGIN表示从第一行开始，{FS=":"}定义当前行以：分割，$3 < 10 判断第三列长度是否小于10，如果是则输出第一列和第三列
awk '{total = $2 + $3 + $4' #计算
```

##cmp 按位对比文件
###格式
```
cmp [-s] file1 file2
```
###常用选项：
- -s  ：将所有的不同点的位组处都列出来。因为 cmp 默认仅会输出第一个发现的不同点。

##diff 按行对比文件
###格式
```
diff [-bBi] from-file to-file
```
常用选项：
- -b  ：忽略一行当中，仅有多个空白的差异(例如 "about me" 与 "about     me" 视为相同
- -B  ：忽略空白行的差异。
- -i  ：忽略大小写的不同。
- -Naur 根据差异制作补丁文件

##patch 使用diff命令产生的文件升级或还原文件
###格式：
-升级
```
patch -pN < patch_file
```
-还原
```
patch -R -pN < patch_file
```
常用选项
- -p  ：后面可以接 取消几层目录 的意思
- -R  ：代表还原，将新的文件还原成原来旧的版本
- -pN代表升级/降级操作需要跳过几层目录，如果使用当前下生成的patch文件，即-p0，如果使用上一级目录下生成的patch文件，即-p1，以此类推.有关pN的描述可参考[linux patch 命令小结](http://blog.csdn.net/wh_19910525/article/details/7515540)

##pr 打印预览
###格式
```
pr 文件名
```
