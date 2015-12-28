#Linux X Window
##X Window主要组件
- x client： app层，处理I/O事件，将需要呈现的画面告知x server
- x server： 管理IO硬件(显示器、键盘、鼠标等)，向x client传递I/O事件，将x client传送的图像数据绘制出来
- X Window Manager： 特殊的x client，负责管理所有的 x client，并提供下列功能：
  - 提供许多的控制元素，包括工作列，背景壁纸的配置
  - 管理虚拟壁纸
  - 提供窗口参数，包括：大小、重叠显示、窗口移动、窗口最小化等
- display manager： 提供图形化的登陆环境，并加载x window manager以及语系数据
>KDE、GNOME、XFCE、twm等的shell都提供X Window Manager和display manager，如GNOME的display manager gdm主要负责提供tty7的登陆UI

##X Window的启动流程
### startx
startx 最重要的任务就是找出使用者或者是系统默认的 X server 与 X client 的配置档，而用户也能够用过 startx 的参数来取代配置档的内容
```sh
startx [X client 参数] -- [X server 参数]
```
####startx的参数
#####X Server 的参数
- 使用 startx 后面接的参数；
- 若无参数，则找寻使用者家目录的文件，亦即 ~/.xserverrc
- 若无上述两者，则以 /etc/X11/xinit/xserverrc
- 若无上述三者，则单纯运行 /usr/bin/X (此即 X server 运行档)
#####X Client 的参数
- 使用 startx 后面接的参数；
- 若无参数，则找寻使用者家目录的文件，亦即 ~/.xinitrc
- 若无上述两者，则以 /etc/X11/xinit/xinitrc
- 若无上述三者，则单纯运行 xterm (此为 X 底下的终端机软件)
### xinit
事实上启动X Window的是xinit
```sh
xinit [client option] -- [server or display option]
```
startx将参数直接传递给xinit供其使用
##X Server配置
###字体
/usr/share/X11/fonts/
###显卡驱动
/usr/lib/xorg/modules/drivers/
###xorg.conf
- Module: 被加载到 X Server 当中的模块 (某些功能的驱动程序),这些模块位于/usr/lib/xorg/modules/extensions/
- InputDevice: 包括输入的 1. 键盘的格式 2. 鼠标的格式，以及其他相关输入设备
- Files: 配置字型所在的目录位置等
- Monitor: 监视器的格式， 主要是配置水平、垂直的升级频率，与硬件有关
- Device: 这个重要，就是显卡芯片组的相关配置了
- Screen: 这个是在萤幕上显示的相关解析度与色彩深度的配置项目，与显示的行为有关
- ServerLayout: 上述的每个项目都可以重覆配置，这里则是此一 X server 要取用的哪个项目值的配置罗
