#/bin/bash
#Program:
#	Program creates three files, which named by user's input 
#	and date command.
#History:
#2007/04/01 andrea First release
# 1. 让使用者输入文件名称，并取得 fileuser 这个变量；
echo -e "I will use 'touch' command to create 3 files." # 纯粹显示资讯
read -p "Please input your filename: " fileuser         # 提示使用者输入

# 2. 为了避免使用者随意按 Enter ，利用变量功能分析档名是否有配置？
filename=${fileuser:-"filename"}           # 开始判断有否配置档名,如果随意按下回车或者fileuser设置为空格符，则默认为fileuser赋值为filename

# 3. 开始利用 date 命令来取得所需要的档名了；
date1=$(date --date='2 days ago' +%Y%m%d)  # 前两天的日期
date2=$(date --date='1 days ago' +%Y%m%d)  # 前一天的日期
date3=$(date +%Y%m%d)                      # 今天的日期
file1=${filename}${date1}                  # 底下三行在配置档名
file2=${filename}${date2}
file3=${filename}${date3}

# 4. 将档名创建吧！
touch "$file1"                             # 底下三行在创建文件
touch "$file2"
touch "$file3"
