﻿#ifndef _SOCK_WRAP_H_
#define _SOCK_WRAP_H_

#include "platform.h"

#if defined(_WIN32_PLATFORM_)
#include <winsock2.h>
typedef SOCKET HSocket;
#endif

#if defined (_LINUX_PLATFORM_)
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef int HSocket;
#define SOCKET_ERROR  (-1)
#define INVALID_SOCKET  0
#endif


typedef struct
{
    int block;
    int sendbuffersize;
    int recvbuffersize;
    int lingertimeout;
    int recvtimeout;
    int sendtimeout;
} socketoption_t;

typedef struct
{
   int nbytes;
   int nresult;
} transresult_t;

int InitializeSocketEnvironment();
void FreeSocketEnvironment();
void GetAddressFrom(sockaddr_in *addr, const char *ip, int port);
void GetIpAddress(char *ip, sockaddr_in *addr);
bool IsValidSocketHandle(HSocket handle);
int GetLastSocketError();

HSocket SocketOpen(int tcpudp);
void SocketClose(HSocket &handle);

int SocketBlock(HSocket hs, bool bblock);
int SocketTimeOut(HSocket hs, int recvtimeout, int sendtimeout, int lingertimeout);

int SocketConnect(HSocket hs, sockaddr_in *paddr);
int SocketBind(HSocket hs, sockaddr_in *addr);
HSocket SocketAccept(HSocket hs, sockaddr_in *addr);
int SocketListen(HSocket hs, int maxconn);

void SocketSend(HSocket hs, const char *ptr, int nbytes, transresult_t &rt);
void SocketRecv(HSocket hs, char *ptr, int nbytes, transresult_t &rt);
void SocketTryRecv(HSocket hs, char *ptr, int nbytes, int milliseconds, transresult_t &rt);
void SocketTrySend(HSocket hs, const char *ptr, int nbytes, int milliseconds, transresult_t &rt);

void SocketClearRecvBuffer(HSocket hs);

class CSockWrap
{
public:
    CSockWrap(int tcpudp);
    ~CSockWrap();
    void SetAddress(const char *ip, int port);
    void SetAddress(sockaddr_in *addr);
    int SetTimeOut(int recvtimeout, int sendtimeout, int lingertimeout);
    int SetBufferSize(int recvbuffersize, int sendbuffersize);
    int SetBlock(bool bblock);

    HSocket  GetHandle () { return m_hSocket;}
    void Reopen(bool bForceClose);
    int Connect();
    void Close();
    transresult_t Send(void *ptr, int nbytes);
    transresult_t Recv(void *ptr, int nbytes );
    transresult_t TrySend(void *ptr, int nbytes, int milliseconds);
    transresult_t TryRecv(void *ptr, int nbytes, int  milliseconds );
    void ClearRecvBuffer();
    HSocket  m_hSocket;
protected:
    
    sockaddr_in m_stAddr;
    int m_tcpudp;
};


#endif 

