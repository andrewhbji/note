# Apache  �W��ӛ� �� -- CGI
CGI(�����W�ܽӿ�[Common Gateway Interface])���x��Web Server���ⲿ���݅f�̳���֮�g�����ķ���,ͨ����ָCGI�������CGI�_��,���ھWվ�ό��F�ӑB������γ��õķ���.

## ��һ��: ����Apache����CGI
### ����һ: ʹ��ScriptAliasָ��CGI����·��
��httpd.conf ? ��VirtualHost �� ʹ��[ScriptAlias](http://man.chinaunix.net/newsoft/ApacheManual/mod/mod_alias.html#scriptalias)ָ��ʹApache���S����һ��ָ��Ŀ��е�CGI����.���͑���Ո���@��·���µ��YԴ�r,Apache�ζ������ęn����CGI���򲢇Lԇ�\��.ScriptAlias��������:
```apache
<VirtualHost *:80>
	ScriptAlias /cgi-bin/ /usr/local/apache/cgi-bin/
</VirtualHost>
```
��Aliasָ�����ƣ�����һ�㶼���ָ��λ�DocumentRootĿ������Ŀ�,�^�e���ScriptAlias����һ�Ӻ��x,��URL·���µ��κ��ęn������ҕ��CGI����.

### ������: ScriptAliasĿ¼�����CGI
��춰�ȫԭ��,CGI����ͨ����������ScriptAliasָ����·����,��˹���T���ԇ���Ŀ����l����ʹ��CGI����.�������Ҫ�L��ScriptAliasĿ������CGI,����Ҫ��httpd.conf��ʹ��Optionsָ���@ʽ�����SCGI����:
```apache
<Directory /usr/local/apache/htdocs/somedir> 
	Options +ExecCGI
</Directory>
```
����,�����Ҫָ����Щ�ęn��CGI,߀��Ҫ��httpd.conf,VirtualHost,Directory����AddHandler ָ��ָ��,�����AddHandlerָ����VServer���Ў�cgi��pl��Y���ęn����CGI
```apache
<Directory /usr/local/apache/htdocs/somedir> 
	AddHandler cgi-script cgi pl
</Directory>
```

### ������: ��.htaccess������
.htaccess��ᘌ�·���M�����õ�һ�N����.Apache���ṩһ���YԴ�r,���ڴ��YԴ���ڵ�Ŀ��Ќ���.htaccess�ęn,�����,�tʹ���е�ָ�����ΊW.AllowOverrideָ��Q����.htaccess�ęn��Ŷ����Ч,���ƶ�����Щָ����Գ��F������,���߲����Sʹ��.���,��Ҫ��httpd.conf������:
```apache
<Directory /usr/local/apache/htdocs/somedir> 
	AllowOverride Options
</Directory>
```
Ȼ����.htaccess������,�Ա�Apache���S��·���µ�CGI�������:
```
Options +ExecCGI
```

## �ڶ���: �؆�Apache,ʹ������Ч
UbuntuServer: 
```shell
sudo /etc/init.d/apache2 restart
```

## ������: CGI����
### CGI���̺ͳ�Ҏ���̵Ĳ:
CGI���������ݔ��ǰ������MIME��͵��^,��Http Header,ָ��Browser���յă������,��:
```http
Content-type: text/html
```
### ��һ��CGI����
�@��CGI������Browser�д�ӡһ������.������Ĵ��a������cgi-binĿ��µ�first.pl�ęn��
```perl
#!/usr/bin/perl
print "Content-type: text/html\n\n";
print "Hello, World.";
```
Ȼ����_Browser��ݔ���ַ:
```http
http://localhost/cgi-bin/first.pl
```
�Ϳ�����Browser�п���һ��Hello,World��

## �m�e
### �ľW�j�L��CGI����,���ܕ��l��������r:

1. ����ݔ��

2. CGI�����Դ�a����"POST Method Not Allowed",�@�r���z��һ������,�����Л]���z©

3. ��"Forbidden"�_�^����Ϣ,�f���Й��ކ��},һ����Ҫ�oCGI�ęn�ڙ�ͨ��,�o��"nobody"���ę���:
	```shell
	chmod a+x first.pl
	```
4. "Internal Server Error"��Ϣ,��Ҫ���Apache error log,�����ҵ�CGI����a�����e�`��Ϣ"Premature end of script headers".������ҪԔ���Ų����ҳ����}

### �Ų醖�}��˼·
1. ·����Ϣ
	��ͨ�^shell���г���r,ĳЩ��Ϣ���Ԅӂ��f�oshell,����·��.����,��CGI����ͨ�^server���Еr,�t�]�д�·��,����CGI���������õ��κγ��򶼱��ָ�����������·��,ʹshell���ҵ�����.ͨ�е���������CGI����ĵĵ�һ��ָ�������(ͨ����Perl),����:
	
	```perl
	#!/usr/bin/perl
	```
���׌��ָ��ָʾ��.
2. ����,�����ѽ��f���^,��������
3. �Z���e�`,�]�e���k��.���^������ʹ��Browser����ǰ,�F���������Мyԇ
4. �鿴���eӛ�

## CGI module��lib
���ʹ��Perl����CGI����,���õ�module�ɲ鿴[CPAN](http://www.cpan.org/),���õ�module��CGI.pm,��Ҳ�Կ��]��CGI::Lite,�����F��һ���ڶ������������б�횵���С���ܼ�
�����C�Z�Ծ���CGI����,�t�кܶ��x��,����֮һ��[CGIC](http://www.boutell.com/cgic/)��

## �����Y��
������Usernet�Mcomp.infosystems.www.authoring.cgi�̈́e��ӑՓCGI���P���},HTML Writers Guild���]���б���һ������Ć��}����YԴ.�����YԴ������http://www.hwg.org/lists/hwg-servers/�ҵ�
����߀������xCGIҎ��,����CGI������������м���,ԭʼ�汾�Ʌ���[NCSA](http://hoohoo.ncsa.uiuc.edu/cgi/interface.html),��һ�����²ݰ�����[Common Gateway Interface RFC project](http://web.golux.com/coar/cgi/)
������һ���]���б�������M�ύCGI���P���}�r,�㑪ԓ�_���ṩ��������Ϣ�Ա�����εİl�F����Q���},��:�l����ʲ�N���},��ϣ���õ�ʲ�N�Y��,�Y���c���������ʲ�N����,���\�еķ�����,CGI������ʲ�N�Z�Ծ�����,������ܾ��ṩ���a.