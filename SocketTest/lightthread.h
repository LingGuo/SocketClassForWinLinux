#ifndef _LIGHTTHREAD_H_
#define _LIGHTTHREAD_H_

#include "platform.h"

#if defined(_WIN32_PLATFORM_)
typedef void *   Mutex_Handle;
#endif

#if defined(_LINUX_PLATFORM_)
typedef pthread_mutex_t   Mutex_Handle;
#endif

class CLightThread
{
public:
    CLightThread() {};
    ~CLightThread()  {};
    static int CreateThread(void ( *proc )( void * ), void *pargs);
    static void EndThread();
    static unsigned int GetCurrentThreadId();
    static void Sleep(unsigned int milliseconds);
    static void DiscardTimeSlice();

};



class CLightThreadMutex
{
public:
    CLightThreadMutex();
    ~CLightThreadMutex();
    int Lock();
    int TryLock(unsigned int dwMilliseconds);
    void Unlock();
private:
    Mutex_Handle m_hMutex;
};

typedef struct
{
    unsigned int errorno;
    char errormsg[512];
} thread_error_t;


class CThreadError
{
typedef struct
{
    thread_error_t threaderror;
    unsigned int threadid;
    void * next;
} internal_thread_error_t;

public:
    CThreadError();
    ~CThreadError();
    void operator=(int errorno);
    void operator=(const char * msg);
    void operator=(thread_error_t & st);
    unsigned int GetLastErrorNo();
    const char *GetLastErrorMsg();
    const thread_error_t *GetLastErrorStruct();
private:
    internal_thread_error_t* m_pStart;
    internal_thread_error_t* allocMemory(unsigned int tid);
    internal_thread_error_t * search(unsigned int tid);
};
#endif