#ifndef _CIOCP_H_
#define _CIOCP_H_
#include "./../export_api.h"

#ifdef _WIN32

#ifndef _WIN32
#define _WIN32
#endif

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Windows.h>
#include <WinSock2.h>
#include <MSWSock.h>

enum IO_TYPE
{
	ACCEPT = 10,
	RECV,
	SEND
};

//数据缓冲区大小
#define DATA_BUFF_SIZE 1024

struct IO_DATA_BASE
{
	//重叠体
	OVERLAPPED overlapped;
	//
	int sockfd;
	//
	WSABUF wsaBuff;
	//操作类型
	IO_TYPE iotype;
};

struct IO_EVENT
{
	union
	{
		void* ptr;
		int sockfd;
	} data;
	IO_DATA_BASE* pIOData;
	DWORD bytesTrans = 0;
};

/**
 * @brief 对iocp的封装
 */
class COMMONLIB_EXPORT CIOCP
{
public:
	CIOCP();

	~CIOCP();

	//将AcceptEX函数加载到内存中
	bool LoadFunc(int Listenint);

	//创建一个IO完成端口(IOCP)
	int Create();

	//销毁IOCP
	void Destory();

	//关联文件sockfd和IOCP
	HANDLE Reg(int sockfd);

	//关联自定义数据地址和IOCP
	HANDLE Reg(int sockfd, void* ptr);

	//投递接收链接任务
	bool PostAccept(IO_DATA_BASE* pIoData);

	//投递接收数据任务
	bool PostRecv(IO_DATA_BASE* pIoData);

	//投递发送数据任务
	bool PostSend(IO_DATA_BASE* pIoData);

	//获取所有任务的状态
	int Wait(IO_EVENT& ioEvent, unsigned int timeout = INFINITE);
private:
	//接收客户端函数指针
	LPFN_ACCEPTEX _AcceptEx = NULL;
	//获得地址函数指针
	LPFN_GETACCEPTEXSOCKADDRS _lpfnGetAcceptExSockaddrs;
	//IOCP完成端口
	HANDLE completion_port_ = NULL;
	//监听的套接字
	int listen_sock_ = -1;
};

#endif //#ifdef _WIN32

#endif //#ifndef _CIOCP_H_
