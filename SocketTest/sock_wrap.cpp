#include "platform.h"
#include <stdio.h>
#include <string.h>

#include "lightthread.h"
#include "sock_wrap.h"
#include "spantime.h"

#define INVALIDSOCKHANDLE   INVALID_SOCKET

#if defined(_WIN32_PLATFORM_)
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")
#define ISSOCKHANDLE(x)  (x!=INVALID_SOCKET)
#define BLOCKREADWRITE      0
#define NONBLOCKREADWRITE   0
#define SENDNOSIGNAL        0
#define ETRYAGAIN(x)     (x==WSAEWOULDBLOCK||x==WSAETIMEDOUT)
#define gxsprintf   sprintf_s

#endif


#if defined(_LINUX_PLATFORM_)
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#define ISSOCKHANDLE(x)    (x>0)
#define BLOCKREADWRITE      MSG_WAITALL
#define NONBLOCKREADWRITE   MSG_DONTWAIT
#define SENDNOSIGNAL        MSG_NOSIGNAL
#define ETRYAGAIN(x)        (x==EAGAIN||x==EWOULDBLOCK)
#define gxsprintf           snprintf

#endif

//���õ�ַ
void GetAddressFrom(sockaddr_in *addr, const char *ip, int port)
{
    memset(addr, 0, sizeof(sockaddr_in));
    addr->sin_family = AF_INET;            /*��ַ����ΪAF_INET*/
    if(ip)
    {
        addr->sin_addr.s_addr = inet_addr(ip);
    }
    else
    {
        /*�����ַΪINADDR_ANY��������ʾ���ص�����IP��ַ����Ϊ�����������ж��������ÿ������Ҳ���ܰ󶨶��IP��ַ��
        �������ÿ��������е�IP��ַ�ϼ�����ֱ����ĳ���ͻ��˽���������ʱ��ȷ�������������ĸ�IP��ַ*/
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
    }
    addr->sin_port = htons(port);   /*�˿ں�*/
}
//�õ��ַ�����ʽ��IP��ַ
void GetIpAddress(char *ip, sockaddr_in *addr)
{
    unsigned char *p =(unsigned char *)( &(addr->sin_addr));
    gxsprintf(ip, 17, "%u.%u.%u.%u", *p,*(p+1), *(p+2), *(p+3) );
}
//�õ��������
int GetLastSocketError()
{
#if defined(_WIN32_PLATFORM_)
    return WSAGetLastError();
#endif

#if defined(_LINUX_PLATFORM_)
    return errno;
#endif
}
//�ж��Ƿ���Socket
bool IsValidSocketHandle(HSocket handle)
{
    return ISSOCKHANDLE(handle);
}
//�ر�Socket
void SocketClose(HSocket &handle)
{
    if(ISSOCKHANDLE(handle))
    {
        #if defined(_WIN32_PLATFORM_)
                closesocket(handle);
        #endif

        #if defined(_LINUX_PLATFORM_)
                close(handle);
        #endif
        handle = INVALIDSOCKHANDLE;
    }
}
//����Э�����ʹ�Socket
HSocket SocketOpen(int tcpudp)
{
    int protocol = 0;
    HSocket hs;
    #if defined(_WIN32_PLATFORM_)
        if(tcpudp== SOCK_STREAM) protocol=IPPROTO_TCP;
        else if (tcpudp== SOCK_DGRAM) protocol = IPPROTO_UDP;
    #endif
    hs = socket(AF_INET, tcpudp, protocol);
    return hs;
}
//����Socket
int SocketConnect(HSocket hs, sockaddr_in *paddr)
{
    return connect(hs,(struct sockaddr *)paddr,sizeof(sockaddr_in));
}
//��Socket
int SocketBind(HSocket hs, sockaddr_in *paddr)
{
    return bind(hs, (struct sockaddr *)paddr, sizeof(sockaddr_in));
}
//����Socket
int SocketListen(HSocket hs, int maxconn)
{
    return listen(hs,maxconn);
}
//����Socket
HSocket SocketAccept(HSocket hs, sockaddr_in *paddr)
{
    #if defined(_WIN32_PLATFORM_)
        int cliaddr_len = sizeof(sockaddr_in);
    #endif
    #if defined(_LINUX_PLATFORM_)
        socklen_t cliaddr_len = sizeof(sockaddr_in);
    #endif
    return accept(hs, (struct sockaddr *)paddr, &cliaddr_len);
}
//
// if timeout occurs, nbytes=-1, nresult=1
// if socket error, nbyte=-1, nresult=-1
// if the other side has disconnected in either block mode or nonblock mode, nbytes=0, nresult=-1
// otherwise nbytes= the count of bytes sent , nresult=0
//����
void SocketSend(HSocket hs, const char *ptr, int nbytes, transresult_t &rt)
{
    rt.nbytes = 0;
    rt.nresult = 0;
    if(!ptr|| nbytes<1) return;

    //Linux: flag can be MSG_DONTWAIT, MSG_WAITALL, ʹ��MSG_WAITALL��ʱ��, socket �����Ǵ�������ģʽ�£�����WAITALL����������
    rt.nbytes = send(hs, ptr, nbytes, BLOCKREADWRITE|SENDNOSIGNAL);
    if(rt.nbytes>0)
    {
        rt.nresult = (rt.nbytes == nbytes)?0:1;
    }
    else if(rt.nbytes==0)
    {
       rt.nresult=-1;
    }
    else
    {
        rt.nresult = GetLastSocketError();
        rt.nresult = ETRYAGAIN(rt.nresult)? 1:-1;
    }
}



// if timeout occurs, nbytes=-1, nresult=1
// if socket error, nbyte=-1, nresult=-1
// if the other side has disconnected in either block mode or nonblock mode, nbytes=0, nresult=-1
//����
void SocketRecv(HSocket hs, char *ptr, int nbytes, transresult_t &rt)
{
    rt.nbytes = 0;
    rt.nresult = 0;
    if(!ptr|| nbytes<1) return;

    rt.nbytes = recv(hs, ptr, nbytes, BLOCKREADWRITE);
    if(rt.nbytes>0)
    {
        return;
    }
    else if(rt.nbytes==0)
    {
       rt.nresult=-1;
    }
    else
    {
        rt.nresult = GetLastSocketError();
        rt.nresult = ETRYAGAIN(rt.nresult)? 1:-1;
    }

}
//  nbytes= the count of bytes sent
// if timeout occurs, nresult=1
// if socket error,  nresult=-1,
// if the other side has disconnected in either block mode or nonblock mode, nresult=-2
void SocketTrySend(HSocket hs, const char *ptr, int nbytes, int milliseconds, transresult_t &rt)
{
    rt.nbytes = 0;
    rt.nresult = 0;
    if(!ptr|| nbytes<1) return;


    int n;
    CMyTimeSpan start;
    while(1)
    {
        n = send(hs, ptr+rt.nbytes, nbytes, NONBLOCKREADWRITE|SENDNOSIGNAL);
        if(n>0)
        {
            rt.nbytes += n;
            nbytes -= n;
            if(rt.nbytes >= nbytes) {    rt.nresult = 0;  break; }
        }
        else if( n==0)
        {
            rt.nresult= -2;
            break;
        }
        else
        {
            n = GetLastSocketError();
            if(ETRYAGAIN(n))
            {
                CLightThread::DiscardTimeSlice();
            }
            else
            {
                rt.nresult = -1;
                break;
            }
        }
        if(start.GetSpaninMilliseconds()>milliseconds)  { rt.nresult= 1; break;}
    }
}
// if timeout occurs, nbytes=-1, nresult=1
// if socket error, nbyte=-1, nresult=-1
// if the other side has disconnected in either block mode or nonblock mode, nbytes=0, nresult=-1
void SocketTryRecv(HSocket hs, char *ptr, int nbytes, int milliseconds, transresult_t &rt)
{
    rt.nbytes = 0;
    rt.nresult = 0;
    if(!ptr|| nbytes<1) return;

    if(milliseconds>2)
    {
        CMyTimeSpan start;
        while(1)
        {
            rt.nbytes = recv(hs, ptr, nbytes, NONBLOCKREADWRITE);
            if(rt.nbytes>0)
            {
               break;
            }
            else if(rt.nbytes==0)
            {
                rt.nresult = -1;
                break;
            }
            else
            {
                rt.nresult = GetLastSocketError();
                if( ETRYAGAIN(rt.nresult))
                {
                   if(start.GetSpaninMilliseconds()>milliseconds)  { rt.nresult= 1; break;}
                   CLightThread::DiscardTimeSlice();
                }
                else
                {
                    rt.nresult = -1;
                    break;
                }
            }

        }
    }
    else
    {
        SocketRecv(hs, ptr, nbytes, rt);
    }
}
//������ջ���
void SocketClearRecvBuffer(HSocket hs)
{
    #if defined(_WIN32_PLATFORM_)
        struct timeval tmOut;
        tmOut.tv_sec = 0;
        tmOut.tv_usec = 0;
        fd_set    fds;
        FD_ZERO(&fds);
        FD_SET(hs, &fds);
        int   nRet = 1;
        char tmp[100];
        int rt;
        while(nRet>0)
        {
            //����Ƿ�ɶ�
            nRet= select(FD_SETSIZE, &fds, NULL, NULL, &tmOut);
            if(nRet>0)
            {
               nRet = recv(hs, tmp, 100,0);
            }
        }
        
    #endif

    #if defined(_LINUX_PLATFORM_)
       char tmp[100];
       while(recv(hs, tmp, 100, NONBLOCKREADWRITE)> 0);
    #endif
}
//����Socket�����������ģʽ
int SocketBlock(HSocket hs, bool bblock)
{
    unsigned long mode;
    if( ISSOCKHANDLE(hs))
    {
        #if defined(_WIN32_PLATFORM_)
                mode = bblock?0:1;
                /*
                �����׽ӿڵ�ģʽ:
                FIONBIO:������ֹ�׽ӿ�s�ķ�����ģʽ
                FIONREAD:ȷ���׽ӿ�s�Զ������������
                SIOCATMARK:ȷʵ�Ƿ����еĴ������ݶ��ѱ�����
                */
                return ioctlsocket(hs,FIONBIO,&mode);
        #endif

        #if defined(_LINUX_PLATFORM_)
                mode = fcntl(hs, F_GETFL, 0);                  //��ȡ�ļ���flagsֵ��
                //���ó�����ģʽ      ������ģʽ
                return bblock?fcntl(hs,F_SETFL, mode&~O_NONBLOCK): fcntl(hs, F_SETFL, mode | O_NONBLOCK);
        #endif
    }
    return -1;
}
//Socket��ʱ����
int SocketTimeOut(HSocket hs, int recvtimeout, int sendtimeout, int lingertimeout)   //in milliseconds
{
    int rt=-1;
    if (ISSOCKHANDLE(hs) )
    {
        rt=0;
        #if defined(_WIN32_PLATFORM_)
                if(lingertimeout>-1)
                {
                    struct linger  lin;
                    lin.l_onoff = lingertimeout;
                    lin.l_linger = lingertimeout ;
                    rt = setsockopt(hs,SOL_SOCKET,SO_DONTLINGER,(const char*)&lin,sizeof(linger)) == 0 ? 0:0x1;
                }
                if(recvtimeout>0 && rt == 0)
                {
                    rt = rt | (setsockopt(hs,SOL_SOCKET,SO_RCVTIMEO,(char *)&recvtimeout,sizeof(int))==0?0:0x2);
                }
                if(sendtimeout>0 && rt == 0)
                {
                    rt = rt | (setsockopt(hs,SOL_SOCKET, SO_SNDTIMEO, (char *)&sendtimeout,sizeof(int))==0?0:0x4);
                }
        #endif

        #if defined(_LINUX_PLATFORM_)
           struct timeval timeout;
                if(lingertimeout>-1)
                {
                    struct linger  lin;
                    lin.l_onoff = lingertimeout>0?1:0;
                    lin.l_linger = lingertimeout/1000 ;
                    rt = setsockopt(hs,SOL_SOCKET,SO_LINGER,(const char*)&lin,sizeof(linger)) == 0 ? 0:0x1;
                }
                if(recvtimeout>0 && rt == 0)
                {
                    timeout.tv_sec = recvtimeout/1000;
                    timeout.tv_usec = (recvtimeout % 1000)*1000;
                    rt = rt | (setsockopt(hs,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==0?0:0x2);
                }
                if(sendtimeout>0 && rt == 0)
                {
                    timeout.tv_sec = sendtimeout/1000;
                    timeout.tv_usec = (sendtimeout % 1000)*1000;
                    rt = rt | (setsockopt(hs,SOL_SOCKET, SO_SNDTIMEO, &timeout,sizeof(timeout))==0?0:0x4);
                }
        #endif
    }
    return rt;
}

//��ʼ��Socket
int InitializeSocketEnvironment()
{
    #if defined(_WIN32_PLATFORM_)
        WSADATA  Ws;
        //Init Windows Socket
        if ( WSAStartup(MAKEWORD(2,2), &Ws) != 0 )
        {
            return -1;
        }
    #endif
    return 0;
}
//�ͷ�Socket
void FreeSocketEnvironment()
{
#if defined(_WIN32_PLATFORM_)
    WSACleanup();
#endif
}
//==============================================================================================================
//================================================================================================================
CSockWrap::CSockWrap(int tcpudp)
{
    memset(&m_stAddr, 0, sizeof(sockaddr_in));
    m_tcpudp = tcpudp;
    m_hSocket = INVALIDSOCKHANDLE;
    Reopen(false);
}


CSockWrap::~CSockWrap()
{
    SocketClose(m_hSocket);
}
void CSockWrap::Reopen(bool bForceClose)
{

    if (ISSOCKHANDLE(m_hSocket) && bForceClose) SocketClose(m_hSocket);
    if (!ISSOCKHANDLE(m_hSocket) )
    {
        m_hSocket=SocketOpen(m_tcpudp);
    }

}
void CSockWrap::SetAddress(const char *ip, int port)
{
    GetAddressFrom(&m_stAddr, ip, port);
}
void CSockWrap::SetAddress(sockaddr_in *addr)
{
    memcpy(&m_stAddr, addr, sizeof(sockaddr_in));
}

int CSockWrap::SetTimeOut(int recvtimeout, int sendtimeout, int lingertimeout)   //in milliseconds
{
  return SocketTimeOut(m_hSocket, recvtimeout, sendtimeout, lingertimeout);
}

int CSockWrap::SetBufferSize(int recvbuffersize, int sendbuffersize)   //in bytes
{
    int rt=-1;
    if (ISSOCKHANDLE(m_hSocket) )
    {
        #if defined(_WIN32_PLATFORM_)
                if(recvbuffersize>-1)
                {
                    rt = setsockopt( m_hSocket, SOL_SOCKET, SO_RCVBUF, ( const char* )&recvbuffersize, sizeof( int ) );
                }
                if(sendbuffersize>-1)
                {
                    rt = rt | (setsockopt(m_hSocket,SOL_SOCKET,SO_SNDBUF,(char *)&sendbuffersize,sizeof(int))==0?0:0x2);
                }
        #endif
    }
    return rt;
}

int CSockWrap::SetBlock(bool bblock)
{
    return SocketBlock(m_hSocket, bblock);
}

int CSockWrap::Connect()
{
    return SocketConnect(m_hSocket,&m_stAddr);
}

transresult_t CSockWrap::Send(void *ptr, int nbytes)
{
    transresult_t rt;
    SocketSend(m_hSocket, (const char *)ptr, nbytes,rt);
    return rt;
}
transresult_t CSockWrap::Recv(void *ptr, int nbytes )
{
    transresult_t rt;
    SocketRecv(m_hSocket, (char *)ptr, nbytes,rt);
    return rt;
}
transresult_t CSockWrap::TrySend(void *ptr, int nbytes, int milliseconds)
{
    transresult_t rt;
    SocketTrySend(m_hSocket, (const char *)ptr, nbytes,milliseconds, rt);
    return rt;
}
transresult_t CSockWrap::TryRecv(void *ptr, int nbytes, int  milliseconds )
{
    transresult_t rt;
    SocketTryRecv(m_hSocket, (char *)ptr, nbytes,milliseconds, rt);
    return rt;
}

void CSockWrap::ClearRecvBuffer()
{
    SocketClearRecvBuffer(m_hSocket);
}

