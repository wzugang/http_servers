#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

__attribute__ ((constructor)) static void so_init(void);
__attribute__ ((destructor)) static void so_deinit(void);

typedef struct
{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
}formattime;

#ifndef __linux__
typedef long long int time_t;
#endif

typedef unsigned short WORD;
typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

#ifndef __linux__
struct timeval
{
	time_t tv_sec; /* 秒 */
	int tv_usec; /* 微秒 */
};
#endif

struct timezone
{
	int tz_minuteswest; /* (minutes west of Greenwich) */
	int tz_dsttime; /* (type of DST correction)*/
};

// struct stat {
//     dev_t         st_dev;       //文件的设备编号
//     ino_t         st_ino;       //节点
//     mode_t        st_mode;      //文件的类型和存取的权限
//     nlink_t       st_nlink;     //连到该文件的硬连接数目，刚建立的文件值为1
//     uid_t         st_uid;       //用户ID
//     gid_t         st_gid;       //组ID
//     dev_t         st_rdev;      //(设备类型)若此文件为设备文件，则为其设备编号
//     off_t         st_size;      //文件字节数(文件大小)
//     unsigned long st_blksize;   //块大小(文件系统的I/O 缓冲区大小)
//     unsigned long st_blocks;    //块数
//     time_t        st_atime;     //最后一次访问时间
//     time_t        st_mtime;     //最后一次修改时间
//     time_t        st_ctime;     //最后一次改变时间(指属性)
// };

// struct utimbuf
// {
//     time_t actime;
//     time_t modtime;
// };

// unistd.h
// struct timespec
// {
// 	time_t tv_sec;
// 	long tv_nsec;
// };

char g_timebuffer[64]={0};
// char g_timebuffer1[32]= "2019-08-25 10:39:30";//星期日，正确
// char g_timebuffer2[32]= "2000-1-1 10:39:30"; //星期六，正确
// char g_timebuffer3[32]= "1752-09-02 10:39:30"; //星期六(有争议)
// char g_timebuffer4[32]= "1752-09-14 10:39:30"; //星期四，正确
// //下面不准
// char g_timebuffer5[32]= "1-1-1 10:39:30"; //星期六，星期四(有争议)
// char g_timebuffer6[32]= "1582-10-4 10:39:30"; //星期四,正确
// char g_timebuffer7[32]= "1582-10-15 10:39:30"; //星期五,正确

// char* gp_timebuffer;

//cal 9 1752
//东八区，时间比世界时快
int time_zone = -8;

#define TIMEHOOKFILE				"timehook.txt"
#define SECONDS_OF_ONE_DAY 			(24*60*60)
#define SECONDS_OF_ONE_HOUR			(60*60)

int get_timehook_configpath(char* buf, int size)
{
	char* p = NULL;
	char *pathvar = getenv("LD_PRELOAD");
	if(NULL == pathvar)
	{
		printf("get env LD_PRELOAD error\n");
		return 0;
	}
	int len = strlen(pathvar);
	if(len + 1 > size)
	{
		printf("get_timehook_configpath buf size error: preload:%s, len:%d, size:%d\n", pathvar, len, size);
		return -1;
	}
	snprintf(buf, len, "%s", pathvar); //拷贝n个字符，后面追加'\0'
	p = buf+len;
	while(*p != '/')
	{
		*p = '\0'; p--;
	}
	
	return 0;
}


ssize_t timeread(int fd, void * buf, size_t count)
{
	int len=0;
	int ret;
	do
	{
		ret = read(fd, buf + len, count - len);
		if(ret > 0)
		{
			len += ret;
		}
		//已经读到文件尾了
		if(0 == ret)
		{
			break;
		}
		//EBADF
		//printf("bad file descriptor\n");
	}while(((-1 == ret) && (errno == EAGAIN || errno == EINTR)) || (len < count));
	
	return len;
}

void timestrim(void *buf)
{
	char* p = buf;
	if(NULL == p)return;
	while(*p != '\0' && *p != '\n' && *p != '\r')p++;
	if('\n' == *p)*p = '\0';
	if('\r' == *p)*p = '\0';
}

static void so_init(void)
{
	char buf[128]={0};
	char file[256]={0};
	int len;
	(void)get_timehook_configpath(file,sizeof(file));
	strcat(file, TIMEHOOKFILE);
    //printf("config file:%s\n", file);
	int fd = open(file, O_RDONLY, 0640);
	if(fd < 0)
	{
		printf("timehook file:%s is not exists\n", file);
		return;
	}
	len = timeread(fd, buf, sizeof(buf)-1);
	if(len > 1)
	{
		timestrim(buf);
		sprintf(g_timebuffer, "%s", buf);
	}
	close(fd);
}

static void so_deinit(void)
{
    //printf("call so deinit.\n");
}

int is_leap_year(int year)
{
	return ((year%4==0&&year%100!=0)||year%400==0)?1:0;
}

int get_year_days(int year)
{
	return (is_leap_year(year))?366:365;
}

int get_month_days(int year,int month)
{
	return (month ==4 || month == 6 || month == 9|| month == 11)?30:((month == 2)?(is_leap_year(year)?29:28):31);
}

int get_yearofday(int year,int month,int day)
{
	int i,days=0;
	for(i=1;i<month;i++)
	{
		days += get_month_days(year,i);
	}
	return (days + day);
}

//高斯--泽勒公式,0为星期日，公元1年1月1日是星期六？，365天是52周+1天
//1752年9月2日之后的那一天并不是1752年9月3日，而是1752年9月14日。也就是说，从1752年9月3日到1752年9月13日的12天并不存在。抹掉这12天是由英国议会做出的决定。星期不变。
//1582年，格里果里十三世教皇才同意了一位业余天文学家的方案，颁发了改儒略历为格里历的法令，其实，改变的实质主要有二：即在当年扣除多余的10天，具体说来说是把1582年10月4日（星期四）后面的那一天，作为10月15日星期五（本应是10月5日星期五）；今后凡不能被400整除的世纪年，如1700年、1800年、1900年等不再作闰年，只有如1600年、2000年等那样可以被400除尽的年份才仍用闰年。这实际意味着在每400年中加了397个闰日，比原先少了整整3天，也说是说，在新的格里历中，一年长度平均是365.2425天，这与实际年长只差25.9秒，足可保证在二三千年内不出差错。
//1752与1582是个特殊的年份，需要特殊处理
int get_weekofday(int year,int month,int day)
{
	/*
	if(year > 1752 || (year == 1752 && month > 9) || (year == 1752 && month == 9 && day >= 14))
	{
		return ((year-1)+((year-1)/4)-((year-1)/100)+((year-1)/400)+ get_yearofday(year, month, day))%7;
	}
	else if(year == 1752 && month == 9 && day < 14 && day > 2)
	{
		return 0;
	}else 
	*/
	if(year > 1582 || (year == 1582 && month > 10) || (year == 1582 && month == 10 && day >= 15))
	{
		return ((year-1)+((year-1)/4)-((year-1)/100)+((year-1)/400)+ get_yearofday(year, month, day))%7;
	} else if(year == 1582 && month == 10 && day < 15 && day > 4)
	{
		return 0;
	}else
	{
		return ((year-1)+((year-1)/4)-((year-1)/100)+((year-1)/400)+ get_yearofday(year, month, day) + (10-7))%7; //星期加1,时间加11天,相差10天(+5,1年1月1日才是星期六)
	}
}

//英国算法
int get_weekofday2(int year,int month,int day)
{
	if(year > 1752 || (year == 1752 && month > 9) || (year == 1752 && month == 9 && day >= 14))
	{
		return ((year-1)+((year-1)/4)-((year-1)/100)+((year-1)/400)+ get_yearofday(year, month, day))%7;
	}
	else if(year == 1752 && month == 9 && day < 14 && day > 2)
	{
		return 0;
	}else
	{
		return ((year-1)+((year-1)/4)-((year-1)/100)+((year-1)/400)+ get_yearofday(year, month, day) + (11-7))%7; //星期加1,时间加12天,相差11天(+5,1年1月1日才是星期六)
	}
}

time_t get_seconds_since1970(int year, int month, int day, int hour, int minute, int second)
{
	time_t seconds;
	time_t days;
	int i;
	days = 0;
	seconds = 0;
	for(i=1970; i < year; i++)
	{
		days += get_year_days(i);
	}
	
	days += (get_yearofday(year, month, day) - 1);
	
	seconds = (hour*SECONDS_OF_ONE_HOUR) + (minute*60) + second;
	seconds += days*SECONDS_OF_ONE_DAY;
	
	return seconds;
}

int get_formattime_fromsecond(time_t t, formattime* f)
{
	formattime ft = {1970,1,1,0,0,0};
	int s;
	if(NULL == f)
	{
		return -1;
	}
	while(t >= (s = (get_year_days(ft.year) * SECONDS_OF_ONE_DAY)))
	{
		t -= s;
		ft.year += 1;
	}
	
	while(t >= (s = (get_month_days(ft.year, ft.month) * SECONDS_OF_ONE_DAY)))
	{
		t -= s;
		ft.month += 1;
	}
	
	while(t >= SECONDS_OF_ONE_DAY)
	{
		t -= SECONDS_OF_ONE_DAY;
		ft.day += 1;
	}
	
	while(t >= SECONDS_OF_ONE_HOUR)
	{
		t -= SECONDS_OF_ONE_HOUR;
		ft.hour += 1;
	}
	
	while(t >= 60)
	{
		t -= 60;
		ft.minute += 1;
	}
	
	ft.second = t;
	memcpy(f, &ft, sizeof(formattime));
	
	return 0;
}

//2010-11-15 10:39:30
int str2time(const char* str, formattime* ft)
{
	sscanf(str, "%d-%d-%d %d:%d:%d", &ft->year, &ft->month, &ft->day, &ft->hour, &ft->minute, &ft->second);
	return 0;
}

int time2str(formattime* ft, char* str)
{
	sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", ft->year, ft->month, ft->day, ft->hour, ft->minute, ft->second);
	return 0;
}

time_t time(time_t *t)
{
	time_t tt;
	formattime ft;
	
	str2time(g_timebuffer, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second);// + (time_zone * SECONDS_OF_ONE_HOUR);
	if(NULL != t)
	{
		*t = tt;
	}
	// printf("time in hook\n");
	return tt;
}

int gettimeofday(struct timeval* tv,struct  timezone *tz )
{
	time_t tt;
	formattime ft;
	if(NULL == tv)
	{
		printf("gettimeofday tv null\n");
		return -1;
	}
	str2time(g_timebuffer, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second);// + (time_zone * SECONDS_OF_ONE_HOUR);
	tv->tv_sec = tt;
	tv->tv_usec = 0;
	if(NULL != tz)
	{
		tz->tz_minuteswest = -480;
		tz->tz_dsttime = 0;
	}
	// printf("gettimeofday in hook\n");
	return 0;
}

// CLOCK_MONOTONIC,单调时间,系统启动到现在的时间,单调的时间与设置无关
// CLOCK_REALTIME, 墙上时间,1970年到现在的UTC,date命令会调用该接口
// CLOCK_REALTIME:系统实时时间,随系统实时时间改变而改变
// CLOCK_MONOTONIC,从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
// CLOCK_PROCESS_CPUTIME_ID,本进程到当前代码系统CPU花费的时间
// CLOCK_THREAD_CPUTIME_ID,本线程到当前代码系统CPU花费的时间
int clock_gettime(int clk_id, struct timespec* tp)
{
	time_t tt;
	formattime ft;
	(void)clk_id;
	if(NULL == tp)
	{
		printf("clock_gettime tp null\n");
		return -1;
	}
	str2time(g_timebuffer, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second);// + (time_zone * SECONDS_OF_ONE_HOUR);
	tp->tv_sec = tt;
	tp->tv_nsec = 0;
	
	// printf("clock_gettime in hook\n");
	return 0;
}

void GetSystemTime(SYSTEMTIME* lpSystemTime)
{
	formattime ft;
	
	str2time(g_timebuffer, &ft);
	lpSystemTime->wYear = ft.year;
	lpSystemTime->wMonth = ft.month;
	lpSystemTime->wDay = ft.day;
	lpSystemTime->wHour = ft.hour;
	lpSystemTime->wMinute = ft.minute;
	lpSystemTime->wSecond = ft.second;
	lpSystemTime->wMilliseconds = 0;
	lpSystemTime->wDayOfWeek = get_weekofday(ft.year, ft.month, ft.day);
}

void GetLocalTime(SYSTEMTIME* lpSystemTime)
{
	formattime ft;
	
	str2time(g_timebuffer, &ft);
	lpSystemTime->wYear = ft.year;
	lpSystemTime->wMonth = ft.month;
	lpSystemTime->wDay = ft.day;
	lpSystemTime->wHour = ft.hour;
	lpSystemTime->wMinute = ft.minute;
	lpSystemTime->wSecond = ft.second;
	lpSystemTime->wMilliseconds = 0;
	lpSystemTime->wDayOfWeek = get_weekofday(ft.year, ft.month, ft.day);
}

#if 0
#include <stdio.h> 
#include <time.h>

void main() 
{
	time_t t;
	t=time(NULL);
	printf("%s\n",ctime(&t));
	struct tm *n=localtime(&t);
	char str[60];
	sprintf(str,"%d/%d/%d %d:%d:%d",n->tm_year+1900,n->tm_mon+1,n->tm_mday,n->tm_hour,n->tm_min,n->tm_sec);
	printf("%s\n",str); 
}

#include <windows.h>
#include <stdio.h>

int main()
{
    SYSTEMTIME currentTime;
    GetSystemTime(&currentTime);
    printf("time: %u/%u/%u %u:%u:%u:%u %d\n",            
     currentTime.wYear,currentTime.wMonth,currentTime.wDay,
     currentTime.wHour,currentTime.wMinute,currentTime.wSecond,
     currentTime.wMilliseconds,currentTime.wDayOfWeek);
    return 0;
}


void print_systime()
{
	SYSTEMTIME currentTime;
	GetSystemTime(&currentTime);
	printf("time: %u/%u/%u %u:%u:%u:%u %d\n",
	currentTime.wYear,currentTime.wMonth,currentTime.wDay,
	currentTime.wHour,currentTime.wMinute,currentTime.wSecond,
	currentTime.wMilliseconds,currentTime.wDayOfWeek);
}

int main()
{
	int i;
	
	#define TIME_SET(i) gp_timebuffer = g_timebuffer##i

	TIME_SET(1);
	print_systime();
	TIME_SET(2);
	print_systime();
	TIME_SET(3);
	print_systime();
	TIME_SET(4);
	print_systime();
	TIME_SET(5);
	print_systime();
	TIME_SET(6);
	print_systime();
	TIME_SET(7);
	print_systime();

    return 0;
}
#endif


int stat(const char *file_name, struct stat *buf)
{
	time_t tt;
	formattime ft;
	(void)file_name;
	if(NULL == buf)
	{
		printf("stat buf null\n");
		return -1;
	}
	str2time(g_timebuffer, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second);
	
	memset(buf, 0, sizeof(struct stat));
    buf->st_atime = tt;
    buf->st_mtime = tt;
    buf->st_ctime = tt;
	
	return 0;
}

int fstat(int filedes, struct stat *buf)
{
	time_t tt;
	formattime ft;
	(void)filedes;
	if(NULL == buf)
	{
		printf("fstat buf null\n");
		return -1;
	}
	str2time(g_timebuffer, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second);
	memset(buf, 0, sizeof(struct stat));
    buf->st_atime = tt;
    buf->st_mtime = tt;
    buf->st_ctime = tt;
	
	return 0;
}

int lstat(const char * pathname, struct stat * buf)
{
	time_t tt;
	formattime ft;
	(void)pathname;
	if(NULL == buf)
	{
		printf("lstat buf null\n");
		return -1;
	}
	str2time(g_timebuffer, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second);
	
	memset(buf, 0, sizeof(struct stat));
    buf->st_atime = tt;
    buf->st_mtime = tt;
    buf->st_ctime = tt;
	
	return 0;
}

// 修改文件时间
#if 0

int utime(const char * filename, struct utimbuf * buf)
{
	time_t tt;
	formattime ft;
	(void)pathname;
	if(NULL == buf)
	{
		printf("lstat buf null\n");
		return -1;
	}
	str2time(g_timebuffer, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second);
	
    buf->st_atime = tt;
    buf->st_mtime = tt;
    buf->st_ctime = tt;
	
	return 0;
}

int futimes(int fd, const struct timeval tv[2])
{
	time_t tt;
	formattime ft;
	(void)pathname;
	if(NULL == buf)
	{
		printf("lstat buf null\n");
		return -1;
	}
	str2time(g_timebuffer, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second);
	
    buf->st_atime = tt;
    buf->st_mtime = tt;
    buf->st_ctime = tt;
	
	return 0;
}

int utimes(const char *filename, const struct timeval times[2])
{
	time_t tt;
	formattime ft;
	(void)pathname;
	if(NULL == buf)
	{
		printf("lstat buf null\n");
		return -1;
	}
	str2time(g_timebuffer, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second);
	
    buf->st_atime = tt;
    buf->st_mtime = tt;
    buf->st_ctime = tt;
	
	return 0;
}

int lutimes(const char *filename, const struct timeval tv[2])
{
	time_t tt;
	formattime ft;
	(void)filename;
	if(NULL == buf)
	{
		printf("lstat buf null\n");
		return -1;
	}
	str2time(g_timebuffer, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second);
	
	tv[0].tv_sec = tt;
	tv[0].tv_usec = 0;
    tv[1].tv_sec = tt;
	tv[1].tv_usec = 0;
	
	return 0;
}

#endif

//gcc -shared -fpic -o timehook.so timehook.c //-fpermissive

//export LD_PRELOAD="/home/wzugang/time/timehook.so"

//export LD_PRELOAD="/desktop/httptools/curl/timehook.so"

//时间戳与校验和不一致







