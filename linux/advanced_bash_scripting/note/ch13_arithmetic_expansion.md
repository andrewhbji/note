[TOC]
# 算数扩展
## 比较差异
### 使用 反引号 的算数扩展
```sh
z=`expr $z + 3`          # The 'expr' command performs the expansion.
```
### 使用 双圆括号 或 let 的算数扩展
```sh
z=$(($z+3))
z=$((z+3))
let z=z+3
let "z += 3"
```
