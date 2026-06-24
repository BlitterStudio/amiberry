// stdafx.cpp : source file that includes just the standard includes
// CAPSImg.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information


#include "stdafx.h"

void GetLocalTime(LPSYSTEMTIME lpSystemTime)
{
    time_t t = time(NULL);
    struct tm *tp = localtime(&t);
    
    lpSystemTime->wYear = tp->tm_year + 1900;
    lpSystemTime->wMonth = tp->tm_mon + 1;
    lpSystemTime->wDayOfWeek = tp->tm_wday;
    lpSystemTime->wDay = tp->tm_mday;
    lpSystemTime->wHour = tp->tm_hour;
    lpSystemTime->wMinute = tp->tm_min;
    lpSystemTime->wSecond = tp->tm_sec;
    // we don't have milliseconds in struct tm
    lpSystemTime->wMilliseconds = 0;
}
