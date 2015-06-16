#include "spantime.h"


CMyTimeSpan::CMyTimeSpan()
{
    getCurrentTimeLong(&m_start);
}
  void CMyTimeSpan::Reset()
  {
    getCurrentTimeLong(&m_start);
  }
  unsigned int CMyTimeSpan::GetSpaninMilliseconds()
  {
    return (unsigned int)(GetSpaninMicroseconds()/1000LL);
  }
  unsigned int CMyTimeSpan::GetSpaninSeconds()
  {
    return (unsigned int)(GetSpaninMicroseconds()/1000000LL);
  }
  unsigned long long CMyTimeSpan::GetSpaninMicroseconds()
  {
   timelong_t end;
   getCurrentTimeLong(&end);
#if defined(_WIN32_PLATFORM_)
     return (end.QuadPart - m_start.QuadPart)/10;
#endif
  #if defined(_LINUX_PLATFORM_)
     return 1000000LL * ( end.tv_sec - m_start.tv_sec ) + end.tv_usec - m_start.tv_usec;
#endif

  }
void CMyTimeSpan::getCurrentTimeLong(timelong_t *tl)
{
#if defined(_WIN32_PLATFORM_)
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    tl->HighPart= ft.dwHighDateTime;
    tl->LowPart = ft.dwLowDateTime;
#endif
#if defined(_LINUX_PLATFORM_)
    gettimeofday( tl, NULL);
#endif
}