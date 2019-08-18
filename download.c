#include <stdio.h>
#include <fcntl.h>		// 可能文件读写函数位置
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>		// 可能文件读写函数位置

#define CURL_STATICLIB

#include "curl/curl.h"
#include "curl/easy.h"

int g_filesize;

size_t writeData(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realSize = size * nmemb;
	int *pfd = (int*)userp;
	
	if(0 != realSize)
	{
		//写文件数据
		g_filesize += size;
		write(*pfd, contents, size);
	}
	
	return realSize;
}

int curlInit()
{
	curl_global_init(CURL_GLOBAL_ALL);
	return 0;
}

int curlFini()
{
	curl_global_cleanup();
	
	return 0;
}

char* curlUserPassword(char* url)
{
	char* p;
	static char userPassword[128] = {0};
	int i = 0;
	char* start = strstr(url, "://");
	char* end = strstr(url, "@");
	
	if(NULL != start && NULL != end)
	{
		p = start;
		while(p != end)
		{
			userPassword[i] = *p; i++, p++;
		}
		start += strlen("://");
		end += strlen("@");
		while(*end != '\0')
		{
			*start = *end; start++, end++;
		}
		*start = *end;
	}

	return userPassword;
}

CURL* curlCreateHandle(char* url)
{
	char* userPassword;
	CURL* curl = curl_easy_init();
	if(NULL == curl)
	{
		return NULL;
	}
	
	userPassword = curlUserPassword(url);
	
	curl_easy_setopt(curl, CURLOPT_URL, url);
	/* 用户密码设置 */
	curl_easy_setopt(curl, CURLOPT_USERPWD, userPassword);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	/// curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 跳过证书检查
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	
	/// 开启SSL
	/// curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	
	/// curl_easy_setopt(curl, CURLOPT_CAPATH, "./server.crt");
	
	/// 打印详细错误信息
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	
	/// 关闭SSL验证
	/// curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	
	/* 设置连接超时，单位：毫秒 */
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 3000);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	
	return curl;
}

int curlCloseHandle(CURL* curl)
{
	curl_easy_cleanup(curl);
	return 0;
}

size_t curlDownloadGetSize(CURL* curl)
{
	CURLcode	ret;
	size_t		filelen = 0;
	
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
	
	do
	{
		ret = curl_easy_perform(curl);
		if(CURLE_OK != ret)
		{
			printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(ret));
			break;
		}
		
		ret = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filelen);
		if(CURLE_OK != ret)
		{
			filelen = 0;
			printf("curl_easy_getinfo() failed: %s\n", curl_easy_strerror(ret));
		}
	}while(0);
	
	curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
	curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
	
	return filelen;
}

int curlCreateFile(char* filename)
{
	// int fd = open(filename, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE);
	int fd = open(filename, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0640);
	if(fd < 0)
	{
		printf("curlFile() create file :%s failed\n", filename);
		return -1;
	}
	
	return fd;
}

int curlCloseFile(int fd)
{
	close(fd);
	return 0;
}

int curlDownload(char* url, char* filename)
{
	CURLcode ret;
	CURL* curl = curlCreateHandle(url);
	if(NULL == curl)
	{
		return -1;
	}
	
	/// int fd = curlCreateFile(filename);
	/// if(fd < 0)
	/// {
	/// 	curlCloseHandle(curl);
	/// 	return -1;
	/// }
	
	FILE *fp = NULL;
	fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		//cout << "File cannot be opened";
		return CURLE_RECV_ERROR;
	}

	/* 回调函数指针 */
	/// curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&fd);
	/// curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024*1024);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 1000);
	
	g_filesize = 0;
	ret = curl_easy_perform(curl);
	if(CURLE_OK != ret)
	{
		printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(ret));
		curlCloseHandle(curl);
		fclose(fp);
		/// curlCloseFile(fd);
		return -1;
	}
	
	curlCloseHandle(curl);
	//// curlCloseFile(fd);
	fclose(fp);
	return g_filesize;
}

// https://github.com/openssl/openssl/archive/OpenSSL_1_0_2c.zip
// OpenSSL_1_0_2c.zip


void curlVersion()
{
	curl_version_info_data *curlinfo = NULL;
	const char *const *proto;
	curlinfo = curl_version_info(CURLVERSION_NOW);
	if(!curlinfo)
		return ;
	if(curlinfo->protocols) {
		for(proto = curlinfo->protocols; *proto; proto++) {
			printf("protocol:%s\n", *proto);
		}
	}
}

int main(int argc, char**argv)
{
	
	if(argc != 3)
	{
		printf("argc:%d, need 3, download url file\n",argc);
		return -1;
	}
	curlInit();
	
	curlVersion();
	curlDownload(argv[1], argv[2]);
	curlFini();
	// 如果大小为0，失败了需要删除
}


// ./download https://localhost/rar/rar.txt rar.txt

// 不使用SSL
// curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
// curl_easy_setopt(curl，CURLOPT_CAPATH，capath);
// 
// 使用openssl工具将其从crt转换为PEM
// openssl x509 -inform DES -in yourdownloaded.crt -out outcert.pem -text
// 
// 获取特定服务器的CA证书的一种方法：
// openssl s_client -showcerts -servername server -connect server：443> cacert.pem
// 
// 查看证书
// openssl x509 -inform PEM -in certfile -text -out certdata
// 
// 如果您想信任该证书，可以将其添加到CA证书库或按照说明独立使用

 // ./download.exe https://codeload.github.com/wzugang/http_servers/zip/master test.zip

// curl: (60) SSL certificate problem: unable to get local issuer certificate

#if 0
void HelloWorld::dohttps(const char url,const chardata){
CURL curl;
CURLcode res;
int result = 1;
string buffer;
curl_global_init;
curl = curl_easy_init;
if
{
curl_slistplist = curl_slist_append(NULL, “Content-Type: text/xml;charset=UTF-8”);
curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
res=curl_easy_setopt( curl, CURLOPT_URL, url );
//设定为不验证证书和host
res=curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
if(res!=CURLE_OK)
{
return ;
}
res=curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
if(res!=CURLE_OK)
{
return ;
}
//res=curl_easy_setopt(curl,CURLOPT_HEADER,1);
//res=curl_easy_setopt(curl,CURLOPT_CAINFO,pCACertFile);
//res=curl_easy_setopt(curl, CURLOPT_POST, 1);//启用POST提交
//res=curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
res=curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
res=curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, HelloWorld::writer );
res=curl_easy_setopt( curl, CURLOPT_WRITEDATA, &buffer );
res=curl_easy_setopt(curl, CURLOPT_VERBOSE, 1); // Used to debug
//curl_easy_setopt(curl, CURLOPT_STDERR, pFILE_error_info); // save error info in the file or stderr
//curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buff); // error_buff used to save error info
//res=curl_easy_setopt(curl, CURLOPT_RETURNTRANSFER, 1);
res = curl_easy_perform( curl );
curl_easy_cleanup( curl );
std::string result=buffer.c_str();
CCLog(“gggggggggg=%s”,result);
}
}

// do test
std::string httpsurl=“https://login.taobao.com/member/login.jhtml”; 1
dohttps(httpsurl.c_str(),httpsdata.c_str());


#endif






