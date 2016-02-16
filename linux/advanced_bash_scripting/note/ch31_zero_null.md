[TOC]
# /dev/zero和/dev/null
## /dev/null
数据黑洞，写入的内容会丢失，也无法读取到任何内容
### 用途
- 用于禁用stdout和stderr
```sh
cat $filename >/dev/null # 禁用stdout
rm $badname 2>/dev/null # 禁用stderr
cat $filename 2>/dev/null >/dev/null # 同时禁用stdout和stderr
```
- 删除文件的内容
```sh
cat /dev/null > /var/log/messages
```
## /dev/zero
与/dev/null类似，写入的内容将丢失，但是可以使用od或者一个16进制编辑器读取内容
### 用途
- 初始化空文件
- 用0填充指定大小的文件
