# 使用 git lfs 上传大文件到 GitHub
分布式版本控制系统Git在版本控制大文件上有所欠缺。最近Github发布了Git扩展GitLargeFileStorage(LFS)，改进了大文件的版本控制，允许开发者在工作流中整合大的二进制文件如音频样本、数据集和视频。

如下图所示，GitHub使用一个专门服务器存储大文件，用户将本地大文件(如psd文件)push到Remote端时，先将文件上传至大文件服务器，然后Remote端会建立一个指向这个大文件的链接
![LFS](./img/GitHub-lfs.gif)

目前普通用户使用Git LFS上传大文件有限额，文件存储限制1GB，每月流量1GB；土豪随意

下面介绍如何在Linux环境下安装使用Git LFS扩展
1. 首先下载[离线安装包](https://github.com/github/git-lfs/releases/download/v1.1.2/git-lfs-linux-amd64-1.1.2.tar.gz)，下载后解压

2. 进入到解压路径，确认二进制文件 git-lfs 和 安装脚本具有可执行权限，然后以root权限运行 install.sh,这个脚本会将git-lfs复制到/usr/local/bin下。

3. 接着在命令行中输入 git lfs install 进行初始化。这时就可以使用 git lfs 扩展了

4. 在上传大文件时，在使用git add 或 提交该文件前，先使用 git lfs track 命令追踪该文件
```sh
$ git lfs track "*.psd"
```

5. 最后在执行 add commit 和 push 操作，这样大文件就可以上传到 github
```sh
$ git add file.psd
$ git commit -m "Add design file"
$ git push origin master
```
