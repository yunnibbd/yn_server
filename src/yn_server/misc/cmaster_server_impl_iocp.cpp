#include "cmaster_server_impl_iocp.h"
#include "cworker_server_impl_iocp.h"
#ifdef _WIN32
#include "cclient.h"
#include "./../component/clog.h"
#include "./../utils/ctools.h"
#include <cstring>

//服务端线程入口函数
void CMasterServerImplIOCP::Main(CThread *pthread)
{
	iocp_.LoadFunc(sockfd_);
	iocp_.Create();
	if (!iocp_.Reg(sockfd_))
	{
		LOG_ERROR("CAbstractMasterServer::Main iocp_.Reg failed\n");
		return;
	}
	//接收新客户端时需要接收数据的缓冲区，理论上只需要下面这么大的空间
	//const int len = 2 * (sizeof(sockaddr_in) + 16);
	//如果还要在接收连接的时候接收额外的数据就需要更大的空间，这里直接设置成了1024
	//不需要客户端连接后立即发送数据的情况下是上面计算的len大小
	io_data_.wsaBuff.buf = buffer_;
	io_data_.wsaBuff.len = sizeof(buffer_);
	iocp_.PostAccept(&io_data_);
	while (pthread->IsRun())
	{
		int ret = iocp_.Wait(io_event_, INFINITE);
		if (ret < 0)
		{
			LOG_WARNING("iocp_.Wait failed");
			continue;
		}
		else if (ret == 0)
		{
			continue;
		}
		// 接受新连接
		if (IO_TYPE::ACCEPT == io_event_.pIOData->iotype)
		{
			// 新客户端加入
			// 调用的是本类的IocpAccept客户端函数
			IocpAccept(io_data_.sockfd);
			// 再次向IOCP投递接收新连接的任务
			/*
				错误记录：投递接收新连接任务如果放在IocpAccept之前会报很多错误
				!: 原因：投递出去的任务会直接完成，如果超出了最大限制数，
					但是还是会建立连接，再调用IocpAccept函数会直接关闭这个连接，
					然后造成一系列WSA函数的报错
			*/
			iocp_.PostAccept(&io_data_);
		}
	}
}

static sockaddr_in client_addr;
static int client_addr_len = sizeof(sockaddr_in);
//新接收的客户端ip
static char ip[16] = { 0 };
//新接收的客户端端口
int port = 0;

bool CMasterServerImplIOCP::IocpAccept(int cSock)
{
	//在这里可以取出ip地址，做别的事情
	if (cSock == INVALID_SOCKET)
	{
		//理论上不会触发
		LOG_ERROR("CAbstractServer IocpAccept error on line %d\n", __LINE__);
		return false;
	}

	setsockopt(cSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&sockfd_, sizeof(int));
	getpeername(cSock, (sockaddr*)&client_addr, &client_addr_len);
	strncpy(ip, inet_ntoa(client_addr.sin_addr), 16);
	port = ntohs(client_addr.sin_port);

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
	return true;
}

//创建多个消息处理客户端并启动
void CMasterServerImplIOCP::Start(int num)
{
	//开启所有消息处理服务端线程
	for (int i = 0; i < num; ++i)
	{
		CAbstractWorkerServer *pcmsg_server = new CWorkerServerImplIocp();
		pcmsg_server->Init();
		pcmsg_server->set_master_server(this);
		pcmsg_server->Start();
		all_cmsg_server_.push_back(pcmsg_server);
		LOG_INFO("start CAbstractWorkerServer %d\n", i);
	}
}

#endif
