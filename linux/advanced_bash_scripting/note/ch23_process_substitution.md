[TOC]
# 进程替换
用于将子进程中一组命令输出到父进程的stdin，进程替换使用/dev/fd/<n>文件将子进程的执行结果交给父进程
## 模板
- \>(command_list)
- <(command_list)
```sh
bash$ echo >(true)
/dev/fd/63

bash$ echo <(true)
/dev/fd/63

bash$ echo >(true) <(true)
/dev/fd/63 /dev/fd/62



bash$ wc <(cat /usr/share/dict/linux.words)
 483523  483523 4992010 /dev/fd/63

bash$ grep script /usr/share/dict/linux.words | wc
    262     262    3601

bash$ wc <(grep script /usr/share/dict/linux.words)
    262     262    3601 /dev/fd/63
```
> 备注: Bash 在两个文件描述符(file descriptors)之间创建了一个管道, --fIn 和 fOut--. true命令的标准输入被连接到 fOut(dup2(fOut, 0)), 然后 Bash 把/dev/fd/fIn 作为参数传给 echo.如果系统的/dev/fd/<n>文件不够时,Bash 会使用临时文件.
