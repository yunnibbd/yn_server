#include "cabstract_master_server.h"
#include "cabstract_worker_server.h"
#include "./../component/cconfig.h"
#include "./../component/cthread.h"
#include "./../component/clog.h"
#include "cclient.h"
#include "./../utils/ctools.h"
#if __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

//初始化事件
void CAbstractMasterServer::Init()
{
	//读取配置文件
	CConfig *p_config = CConfig::Get();
	max_client_num_ = p_config->GetIntDefault("MaxClientNum", 1000);
	//客户端发送缓冲区和接受缓冲区的大小
	send_buf_size_ = p_config->GetIntDefault("SendBufSize", 8192);
	recv_buf_size_ = p_config->GetIntDefault("RecvBufSize", 8192);
	//心跳超时配置读取
	heart_check_timeout_ = p_config->GetIntDefault("HeartCheckTimeout", 10000);
	//初始化监听socket
	InitSocket();
	//让本类Main函数作为一个线程启动
	thread_.Start(
		nullptr,
		[this](CThread *pthread) {
			Main(pthread);
		},
		nullptr
	);
}

//客户端加入事件
void CAbstractMasterServer::OnJoin(CClient *pclient)
{
	if (client_num_ < max_client_num_)
	{
		//获取最小客户端的消息处理服务端加入进去
		CAbstractWorkerServer *serv = nullptr;
		int min_num = 99999;
		for (auto &pserv : all_cmsg_server_)
		{
			auto num = pserv->AllClientNum();
			if (num < min_num)
			{
				serv = pserv;
				min_num = num;
			}
		}
		if (serv)
			serv->AddClientToClientsBuffer(pclient);
	}
	else
	{
		//当前服务端接受客户端达到上限
		CTools::CloseSocket(pclient->sockfd());
	}
	++client_num_;
	LOG_DEBUG("当前有%d个客户端\n", (int)client_num_);
}

//客户端离开事件
void CAbstractMasterServer::OnLeave(CClient *pclient)
{
	--client_num_;
	LOG_DEBUG("当前有%d个客户端\n", (int)client_num_);
}

//初始化socket
void CAbstractMasterServer::InitSocket()
{
	if (server_port_ == -1)
	{
		LOG_ERROR("server port not set\n");
		return;
	}
	sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd_ == -1)
	{
		LOG_ERROR("socket create failed\n");
		return;
	}
	if (!CTools::MakeReuseAddr(sockfd_))
	{
		LOG_ERROR("socket reuse addr failed\n");
		return;
	}
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(server_port_);
	sin.sin_addr.s_addr = inet_addr(server_ip_);
	if (::bind(sockfd_, (sockaddr*)&sin, sizeof(sin)) == -1)
	{
		LOG_ERROR("socket bind failed\n");
		return;
	}
	if (::listen(sockfd_, 512) == -1)
	{
		LOG_ERROR("socket listen failed\n")
			return;
	}
	LOG_INFO("socket server start!\n");
}
