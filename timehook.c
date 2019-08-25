#include <stdio.h>
#include <string.h>
//#include <time.h>

typedef struct
{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
}formattime;

typedef long long int time_t;

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

#define SECONDS_OF_ONE_DAY 			(24*60*60)
#define SECONDS_OF_ONE_HOUR			(60*60)

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

char g_timebuffer1[32]= "2019-08-25 10:39:30";//星期日，正确
char g_timebuffer2[32]= "2000-1-1 10:39:30"; //星期六，正确
char g_timebuffer3[32]= "1752-09-02 10:39:30"; //星期六(有争议)
char g_timebuffer4[32]= "1752-09-14 10:39:30"; //星期四，正确

//下面不准
char g_timebuffer5[32]= "1-1-1 10:39:30"; //星期六，星期四(有争议)
char g_timebuffer6[32]= "1582-10-4 10:39:30"; //星期四,正确
char g_timebuffer7[32]= "1582-10-15 10:39:30"; //星期五,正确

char* gp_timebuffer;

//cal 9 1752


//东八区，时间比世界时快
int time_zone = -8;

time_t time(time_t *t)
{
	time_t tt;
	formattime ft;
	
	str2time(g_timebuffer1, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second) + (time_zone * SECONDS_OF_ONE_HOUR);
	if(NULL != t)
	{
		*t = tt;
	}
	printf("time in hook\n");
	return tt;
}

struct timeval
{
	time_t tv_sec; /* 秒 */
	int tv_usec; /* 微秒 */
};

struct timezone
{
	int tz_minuteswest; /* (minutes west of Greenwich) */
	int tz_dsttime; /* (type of DST correction)*/
};

int gettimeofday(struct timeval* tv,struct  timezone *tz )
{
	time_t tt;
	formattime ft;
	if(NULL == tv)
	{
		printf("gettimeofday tv null\n");
		return -1;
	}
	str2time(g_timebuffer1, &ft);
	tt = get_seconds_since1970(ft.year, ft.month, ft.day, ft.hour, ft.minute, ft.second) + (time_zone * SECONDS_OF_ONE_HOUR);
	tv->tv_sec = tt;
	tv->tv_usec = 0;
	if(NULL != tz)
	{
		tz->tz_minuteswest = -480;
		tz->tz_dsttime = 0;
	}
	printf("gettimeofday in hook\n");
	return 0;
}

void GetSystemTime(SYSTEMTIME* lpSystemTime)
{
	formattime ft;
	
	str2time(g_timebuffer1, &ft);
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
	
	str2time(g_timebuffer1, &ft);
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



//gcc -shared -fpic -o timehook.so timehook.c //-fpermissive

//export LD_PRELOAD="/home/wzugang/time/timehook.so"

//export LD_PRELOAD="/desktop/httptools/curl/timehook.so"







