#include "cmaster_server_impl_epoll.h"
#if __linux__
#include "./../component/cthread.h"
#include "./../net/cepoll.h"
#include "./../component/clog.h"
#include "cclient.h"
#include "./../utils/ctools.h"
#include "cworker_server_impl_epoll.h"
#include <cstring>

//服务端线程入口函数
void CMatserServerImplEpoll::Main(CThread *pthread)
{
	CEpoll ep;
	ep.Create(1);
	ep.Ctl(EPOLL_CTL_ADD, sockfd_, EPOLLIN);
	while (pthread->IsRun())
	{

		int ret = ep.Wait(1);
		if (ret < 0)
		{
			LOG_WARNING("EasyEpollServer::OnRun error\n");
			continue;
		}
		auto events = ep.events();
		for (int i = 0; i < ret; ++i)
		{
			if (events[i].data.fd == sockfd_)
			{
				if (events[i].events & EPOLLIN)
				{
					Accept();
				}
			}
		}
	}
}

//创建多个消息处理服务端并启动(重载决定使用什么iocp, epoll)
void CMatserServerImplEpoll::Start(int num)
{
	//开启所有消息处理服务端线程
	for (int i = 0; i < num; ++i)
	{
		CAbstractWorkerServer *pcmsg_server = new CWorkerServerImplEpoll();
		pcmsg_server->Init();
		pcmsg_server->set_master_server(this);
		pcmsg_server->Start();
		all_cmsg_server_.push_back(pcmsg_server);
		LOG_INFO("start CAbstractWorkerServer %d\n", i);
	}
}


//用于存放接收的客户端的地址
static sockaddr_in client_addr;
//客户端地址的长度
static socklen_t client_addr_len = sizeof(sockaddr_in);
//新接收的客户端ip
static char ip[16] = { 0 };
//新接收的客户端端口
int port = 0;

//接受客户端
void CMatserServerImplEpoll::Accept()
{
	if (sockfd_ == -1) 
		return;
	int cSock = -1;
	cSock = accept(sockfd_, (struct sockaddr*) & client_addr, &client_addr_len);
	if (cSock == -1)
	{
		LOG_WARNING("CMatserServerImplEpoll accept error\n");
	}
	else
	{
		strncpy(ip, inet_ntoa(client_addr.sin_addr), 16);
		port = ntohs(client_addr.sin_port);
		if (!ip[0] || port == 0)
			return;
		LOG_DEBUG("client %s:%d join\n", ip, port);

		if (client_num_ < max_client_num_)
		{
			//未达到客户端上限，根据socket创建新客户端
			CClient *pclient = new CClient(cSock,
				send_buf_size_, recv_buf_size_,
				heart_check_timeout_);
			//赋值客户端id(测试用)
			pclient->test_id = client_num_;
			//设置client为websocket
			pclient->SetSocketType(socket_type_);
			pclient->SetProtocalType(protocal_type_);
			//触发客户端加入事件
			OnJoin(pclient);
		}
		else
			//当前服务端接受客户端达到上限
			CTools::CloseSocket(cSock);
	}
}

#endif
