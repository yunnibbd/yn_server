#include "cabstract_worker_client.h"
#include "./../component/clog.h"
#include "./../component/cthread.h"
#include "./../utils/ctools.h"
#include <cstring>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

void CAbstractWorkerClient::Init()
{
	//初始化基类
	CAbstractClient::Init();
	
	InitSocket();
}

//关闭本客户端
void CAbstractWorkerClient::Close()
{
	//只关闭socket, 发送缓冲区和接受缓冲区中的数据还要继续处理
	CAbstractClient::CloseSocket();
	is_connected_ = false;
}

//发送数据的线程入口函数
bool CAbstractWorkerClient::Main(int wait_msec)
{
	if (is_connected_)
	{
		//处理事件
		ProcessEvents(wait_msec);
		//处理数据
		ProcessMsg();
		return true;
	}
	else
	{
		//与服务端断开了连接, 要重新建立连接
		CheckConnect();
	}
	return true;
}

//处理收到的消息
void CAbstractWorkerClient::ProcessMsg()
{
	if (!is_connected_)
		return;
	/////////////////////////////////////////////////////
	///!!!后期此处需要填充代码!!!
	//收到数据回调
	if (NeedRead())
		ReadCB(p_recv_buf_->Data(), p_recv_buf_->DataLen());
	/////////////////////////////////////////////////////
}

//初始化socket
void CAbstractWorkerClient::InitSocket()
{
	sockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd_ == -1)
	{
		LOG_ERROR("socket create failed\n");
		return;
	}
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(server_port_);
}

//开始连接服务端
bool CAbstractWorkerClient::StartConnect()
{
	if (is_connected_) 
		return false;
	if (sockfd_ == -1)
		InitSocket();
	if (server_ip_[0] == 0 || server_port_ == 0)
	{
		LOG_WARNING("server_ip or server_port is not set\n");
		return false;
	}
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(server_port_);
	sin.sin_addr.s_addr = inet_addr(server_ip_);
	if (::connect(sockfd_, (sockaddr*)&sin, sizeof(sin)) == -1)
	{
		LOG_WARNING("can not connect server %s:%d\n", server_ip_, server_port_);
		return false;
	}
	is_connected_ = true;
	OnConnected();
	return true;
}

//检查连接的执行函数, 如果断开连接就开始重连接
void CAbstractWorkerClient::CheckConnect()
{
	if (!is_connected_)
	{
		//已经断开了连接, 此函数不保证上次连接是否已经释放了所有内存资源
		//开始重新连接, 在此之前可以重新设置服务端的ip和port
		if (!StartConnect())
		{
			//连接失败
			LOG_INFO("reconnect %s:%d failed\n", server_ip_, server_port_);
			return;
		}
		//连接成功
		//触发连接成功事件
		OnConnected();
		LOG_INFO("reconnect %s:%d success\n", server_ip_, server_port_);
	}
}

//设置要连接的服务端ip
void CAbstractWorkerClient::set_server_ip(const char *ip)
{
	strncpy(server_ip_, ip, 15);
}
