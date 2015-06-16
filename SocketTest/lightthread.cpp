#include <stdio.h>
#include <time.h>
#include <string.h>
#include "lightthread.h"

#if defined(_WIN32_PLATFORM_)
#include <windows.h>
#include <process.h>    /* _beginthread, _endthread */
#define gxstrcpy(d,n,s) strcpy_s(d,n,s)
#endif

#if defined(_LINUX_PLATFORM_)
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#define gxstrcpy(d,n,s) strncpy(d,s,n)
#define THREAD_IDLE_TIMESLICE_MS   20
#endif

#define GX_UNDEFINED      0xffffffff
#define GX_S_OK           0x00000000


#include "spantime.h"

#if defined(_LINUX_PLATFORM_)


typedef struct
{
   void (* proc)( void * ) ;
   void * pargs;
} _threadwraper_linux_t;

void * _ThreadWraper_Linux(void *pargs)
{
    _threadwraper_linux_t *pth= (_threadwraper_linux_t *)pargs;
    pth->proc(pth->pargs);
    delete[] pth;
    return NULL;
}

int CLightThread::CreateThread(void ( *proc )( void * ), void *pargs)
{
   pthread_t ntid;
   _threadwraper_linux_t* pthreadwraper = new _threadwraper_linux_t[1];
   pthreadwraper[0].proc = proc;
   pthreadwraper[0].pargs = pargs;
   return pthread_create(&ntid, NULL, _ThreadWraper_Linux, pthreadwraper);
}

void CLightThread::DiscardTimeSlice()
{
    usleep(THREAD_IDLE_TIMESLICE_MS*1000);
}


void CLightThread::EndThread()
{
   pthread_exit(NULL);
}

unsigned int CLightThread::GetCurrentThreadId()
{
   return pthread_self();
}
void CLightThread::Sleep(unsigned int milliseconds)
{
    if(milliseconds>=1000)
    {
       unsigned int s = milliseconds/1000;
       unsigned int us = milliseconds  - s*1000;
       sleep(s);
       if(us>0)  usleep(us*1000);
    }
    else
    {
       usleep(milliseconds*1000);
    }
}
//=====================================================================================
CLightThreadMutex::CLightThreadMutex()
{
    pthread_mutex_init(&m_hMutex, NULL);
}
CLightThreadMutex::~CLightThreadMutex()
{
    pthread_mutex_destroy(&m_hMutex);
}
int CLightThreadMutex::Lock()
{
    return pthread_mutex_lock(&m_hMutex) == 0 ?0:-1;
}
int CLightThreadMutex::TryLock(unsigned int dwMilliseconds)
{
    // The function pthread_mutex_trylock() returns zero if a lock on the mutex object referenced by mutex is acquired. Otherwise, an error number is returned to indicate the error.
    unsigned int us= dwMilliseconds*1000;
    int rt = pthread_mutex_trylock(&m_hMutex);
    if( rt == EBUSY)
    {
        CMyTimeSpan start;
        while(rt == EBUSY)
        {
            if( start.GetSpaninMilliseconds()>dwMilliseconds)
            {
                rt = -1;
            }
            else
            {
                usleep(20000);         //sleep  20ms
                rt = pthread_mutex_trylock(&m_hMutex);
            }
        }
    }
    return rt;
}

void CLightThreadMutex::Unlock()
{
     pthread_mutex_unlock(&m_hMutex);
}
#endif



#if defined(_WIN32_PLATFORM_)

int CLightThread::CreateThread(void( *proc )( void * ), void *pargs)
{
     return _beginthread( proc, 0, pargs );
}
void CLightThread::EndThread()
{
   _endthread();
}

unsigned int CLightThread::GetCurrentThreadId()
{
    return ::GetCurrentThreadId();
}
void CLightThread::Sleep(unsigned int miniseconds)
{
    ::Sleep(miniseconds);
}

void CLightThread::DiscardTimeSlice()
{
    ::SwitchToThread();
}
//=====================================================================================
//=====================================================================================
CLightThreadMutex::CLightThreadMutex()
{        
    m_hMutex = CreateMutexA(NULL,FALSE,NULL);
}

CLightThreadMutex::~CLightThreadMutex()
{
    if(m_hMutex)   CloseHandle(m_hMutex);
}
int CLightThreadMutex::Lock()
{
    if( m_hMutex && WaitForSingleObject(m_hMutex, INFINITE)==WAIT_OBJECT_0) return 0;
    return -1;
}
int CLightThreadMutex::TryLock(unsigned int dwMilliseconds)
{
  if( m_hMutex&& WaitForSingleObject(m_hMutex, dwMilliseconds) ==WAIT_OBJECT_0) return 0;

  return -1;

}
void CLightThreadMutex::Unlock()
{
    if(m_hMutex) ReleaseMutex(m_hMutex);
}
#endif

//=====================================================================================================
CThreadError::CThreadError()
{
    m_pStart = NULL;
}
CThreadError::~CThreadError()
{
    internal_thread_error_t *temp;
    while(m_pStart)
    {
        temp = m_pStart;
        m_pStart = (internal_thread_error_t*)m_pStart->next;
        delete temp;
    }
}
void CThreadError::operator=(int errorno)
{
    unsigned int tid = CLightThread::GetCurrentThreadId();
    internal_thread_error_t *temp = search(tid);
    if(!temp)
    {
        temp = allocMemory(tid);
    }
    temp->threaderror.errorno = errorno;
    temp->threaderror.errormsg[0] = '\0';
}
void CThreadError::operator=(const char * msg)
{
    unsigned int tid = CLightThread::GetCurrentThreadId();
    internal_thread_error_t *temp = search(tid);
    if(!temp)
    {
        temp = allocMemory(tid);
    }
    temp->threaderror.errorno = GX_UNDEFINED;
    gxstrcpy(temp->threaderror.errormsg, 510, msg);
}
void CThreadError::operator=(thread_error_t & st)
{
    unsigned int tid = CLightThread::GetCurrentThreadId();
    internal_thread_error_t *temp = search(tid);
    if(!temp)
    {
        temp = allocMemory(tid);
    }
    memcpy(&temp->threaderror, &st, sizeof(thread_error_t));
}
unsigned int CThreadError::GetLastErrorNo()
{
    unsigned int tid = CLightThread::GetCurrentThreadId();
    internal_thread_error_t *temp = search(tid);
    return temp?temp->threaderror.errorno:GX_S_OK;
}
const char *CThreadError::GetLastErrorMsg()
{
    unsigned int tid = CLightThread::GetCurrentThreadId();
    internal_thread_error_t *temp = search(tid);
    return temp?(const char*)temp->threaderror.errormsg:NULL;
}
const thread_error_t *CThreadError::GetLastErrorStruct()
{
    unsigned int tid = CLightThread::GetCurrentThreadId();
    internal_thread_error_t *temp = search(tid);
    return temp?(const thread_error_t *)(&(temp->threaderror)):NULL;
}

CThreadError::internal_thread_error_t* CThreadError::allocMemory(unsigned int tid)
{
    internal_thread_error_t *temp = new internal_thread_error_t;
    temp->threadid = tid;
    temp->next = m_pStart;
    m_pStart = temp;
    return temp;
}
CThreadError::internal_thread_error_t * CThreadError::search(unsigned int tid)
{
    internal_thread_error_t *temp = m_pStart;
    while(temp)
    {
        if(temp->threadid == tid) break;
        temp->next = (void *)temp;
    }
    return temp;
}