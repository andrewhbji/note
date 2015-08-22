# Apache  學習記錄 二 -- CGI
CGI(公共網管接口[Common Gateway Interface])定義了Web Server和外部內容協商程序之間交互的方法,通常是指CGI程序或者CGI腳本,是在網站上實現動態頁面的最簡單常用的方法.

## 第一步: 配置Apache允许CGI
### 方法一: 使用ScriptAlias指定CGI程序路径
在httpd.conf ? 和VirtualHost 中 使用[ScriptAlias](http://man.chinaunix.net/newsoft/ApacheManual/mod/mod_alias.html#scriptalias)指令使Apache允許執行一個指定目錄中的CGI程序.當客戶端請求這個路徑下的資源時,Apache嘉定其中文檔都是CGI程序并嘗試運行.ScriptAlias配置如下:
```apache
<VirtualHost *:80>
	ScriptAlias /cgi-bin/ /usr/local/apache/cgi-bin/
</VirtualHost>
```
和Alias指令相似，兩者一般都用於指定位於DocumentRoot目錄以外的目錄,區別在於ScriptAlias多了一層含義,即URL路徑下的任何文檔都會被視作CGI程序.

### 方法二: ScriptAlias目录以外的CGI
由於安全原因,CGI程序通常被限制在ScriptAlias指定的路徑下,如此管理員可以嚴格的控制誰可以使用CGI程序.但是如果要訪問ScriptAlias目錄以外的CGI,就需要在httpd.conf中使用Options指令顯式的允許CGI執行:
```apache
<Directory /usr/local/apache/htdocs/somedir> 
	Options +ExecCGI
</Directory>
```
另外,如果需要指出哪些文檔是CGI,還需要在httpd.conf,VirtualHost,Directory中用AddHandler 指令指出,下面的AddHandler指令告訴Server所有帶cgi或pl後綴的文檔都是CGI
```apache
<Directory /usr/local/apache/htdocs/somedir> 
	AddHandler cgi-script cgi pl
</Directory>
```

### 方法三: 在.htaccess中配置
.htaccess是針對路徑進行配置的一種方法.Apache在提供一個資源時,會在此資源所在的目錄中尋找.htaccess文檔,如果有,則使其中的指令生次奧.AllowOverride指令決定了.htaccess文檔是哦福有效,它制定了那些指令可以出現在其中,或者不允許使用.為此,需要在httpd.conf中配置:
```apache
<Directory /usr/local/apache/htdocs/somedir> 
	AllowOverride Options
</Directory>
```
然後在.htaccess中配置,以便Apache允許此路徑下的CGI程序執行:
```
Options +ExecCGI
```

## 第二步: 重啟Apache,使配置生效
UbuntuServer: 
```shell
sudo /etc/init.d/apache2 restart
```

## 第三步: CGI編程
### CGI編程和常規編程的差異:
CGI程序的所有輸出前面必須有MIME類型的頭,即Http Header,指明Browser接收的內容類型,如:
```http
Content-type: text/html
```
### 第一個CGI程序
這個CGI程序在Browser中打印一行文字.把下面的代碼保存在cgi-bin目錄下的first.pl文檔中
```perl
#!/usr/bin/perl
print "Content-type: text/html\n\n";
print "Hello, World.";
```
然後打開Browser并輸入地址:
```http
http://localhost/cgi-bin/first.pl
```
就可以在Browser中看到一行Hello,World了

## 糾錯
### 從網絡訪問CGI程序,可能會發生四中情況:

1. 正常輸出

2. CGI程序的源碼或者"POST Method Not Allowed",這時重新檢查一下配置,看看有沒有遺漏

3. 以"Forbidden"開頭的消息,說明有權限問題,一般需要給CGI文檔授權通常,給予"nobody"足夠的權限:
	```shell
	chmod a+x first.pl
	```
4. "Internal Server Error"消息,需要查閱Apache error log,可以找到CGI程序產生的錯誤信息"Premature end of script headers".對此需要詳細排查已找出問題

### 排查問題的思路
1. 路徑信息
	當通過shell執行程序時,某些信息會自動傳遞給shell,比如路徑.但是,在CGI程序通過server執行時,則沒有此路徑,所以CGI程序中引用的任何程序都必須指定其完成整的路徑,使shell能找到他們.通行的做法是在CGI程序的的第一行指明解釋器(通常是Perl),例如:
	
	```perl
	#!/usr/bin/perl
	```
必須讓它指向指示器.
2. 權限,上面已經說明過,不再冗述
3. 語法錯誤,沒別的辦法.不過可以在使用Browser執行前,現在命令行中測試
4. 查看出錯記錄

## CGI module和lib
如果使用Perl編寫CGI程序,可用的module可查看[CPAN](http://www.cpan.org/),常用的module是CGI.pm,可也以考慮用CGI::Lite,它實現了一個在多數程序中所有必須的最小功能集
如果用C語言編寫CGI程序,則有很多選擇,其中之一是[CGIC](http://www.boutell.com/cgic/)庫

## 更多資料
可以在Usernet組comp.infosystems.www.authoring.cgi和別人討論CGI相關問題,HTML Writers Guild的郵件列表是一個優秀的問題解答資源.更多資源可以在http://www.hwg.org/lists/hwg-servers/找到
另外還可以閱讀CGI規範,其中CGI程序操作的所有細節,原始版本可參考[NCSA](http://hoohoo.ncsa.uiuc.edu/cgi/interface.html),另一個更新草案參考[Common Gateway Interface RFC project](http://web.golux.com/coar/cgi/)
當你向一個郵件列表或者新聞組提交CGI相關問題時,你應該確保提供了足夠的信息以便更簡單的發現并解決問題,如:發生了什麼問題,你希望得到什麼結果,結果與你的期望有什麼出入,你運行的服務器,CGI程序用什麼語言編寫的,如果可能就提供代碼.