// SocketTest.cpp : 定义控制台应用程序的入口点。

#include "stdafx.h"
#include "string.h"
#include "sock_wrap.h"
#include "lightthread.h"
#include <iostream>

#if defined(_WIN32_PLATFORM_)
#define MYSLEEP(n) Sleep(n)
#endif
#if defined (_LINUX_PLATFORM_)
#define MYSLEEP(n) usleep(n*1000)
#endif

CLightThreadMutex myMutex;
int g_num=0;
void RecvData(void * timenum)
{
    
    int flag=myMutex.Lock();
    int tn=*(int *)timenum;
    std::cout<<"第"<<tn<<"次"<<std::endl;
    delete [] timenum;
    if(flag==0)
        std::cout<<"進入進程"<<std::endl;
    CSockWrap myclient=CSockWrap(SOCK_STREAM);
    myclient.SetAddress("192.168.1.154",4060);
    myclient.SetBlock(false);
    int connflag= myclient.Connect();
    fd_set fd;
    if(connflag==0)
        std::cout<<"SOCK連接成功"<<std::endl;
    else
    {
    	int ierrno=GetLastSocketError();
    	std::cout<<"連接返回代碼："<<connflag<<std::endl;
    	std::cout<<"系統錯誤代碼："<<ierrno<<std::endl;
		#if defined(_WIN32_PLATFORM_)
    	if( ierrno==WSAEWOULDBLOCK)
		#endif
		#if defined (_LINUX_PLATFORM_)
    	if( ierrno==EINPROGRESS)
		#endif
    	    {
    	       std::cout<<"進入非阻塞模式"<<std::endl;

    	       FD_ZERO(&fd);
    	       FD_SET(myclient.m_hSocket,&fd);
    	       timeval timeout;
               #if defined (_LINUX_PLATFORM_)
    	       timeout.tv_sec=60*2;
               #endif

               #if defined (_WIN32_PLATFORM_)
    	       timeout.tv_sec=60*2*1000;
               #endif

    	       int selectflag=select(myclient.m_hSocket+1,&fd,NULL,NULL,&timeout);
    	       if(selectflag==0)
    	       {
                   #if defined (_LINUX_PLATFORM_)
    	    	   std::cout<<"等待超時"<<std::endl;
                   #endif
    	       }
    	    }
    }
    if(FD_ISSET(myclient.m_hSocket,&fd))
    {
		myclient.SetBufferSize(1024,1024);
		char renvchar[1024];
		memset(renvchar,0,1024*sizeof(char));
		transresult_t result=myclient.Recv((void *)renvchar,1023);
		char tmp='\n';
		int i=0;
		while(1)
		{
			++i;
			if(renvchar[i]=='\0')
				break;
			if(renvchar[i]==13)
			{
				renvchar[i]=tmp;
			}
		}
		if(result.nbytes>0)
		{
			printf("接收字符串爲：%s",renvchar);
		}
		else
		{
			std::cerr<<"錯誤代碼爲："<<result.nresult<<std::endl;
		}
    }
    else
    {
    	std::cout<<"接收數據失敗"<<std::endl;

    }
    ++g_num;
    myMutex.Unlock();
}

int main(void)
{

    int initflag=InitializeSocketEnvironment();
    if(initflag==0)
        std::cout<<"SOCK初始化成功"<<std::endl;
    
    
    for(int i=0;i<3;++i)
    {
        CLightThread myThread; 
        //const int *tmp(i);
        myThread.CreateThread(RecvData,(void *)new int(i+1)); 
      
    }
    for(;;)
    {
        Sleep(500);
        if(g_num>=3)
            break;
    }
    FreeSocketEnvironment();
	#if defined(_WIN32_PLATFORM_)
    system("pause");
	#endif
	return 0;
}


