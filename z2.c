#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main()
{
    time_t now;
    struct tm *sp;

    time(&now);

    now -= 8 * 60 * 60;
    
    sp = gmtime(&now);

    printf("%d/%d/%02d %d:%02d %s\n",
        sp->tm_mon + 1,
        sp->tm_mday,
        sp->tm_year + 1900,
        sp->tm_hour,
        sp->tm_min,
        "Pacific Standard Time, PST");

    exit(0);
}