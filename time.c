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

