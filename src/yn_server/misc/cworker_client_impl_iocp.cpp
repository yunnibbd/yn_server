#include "cworker_client_impl_iocp.h"
#ifdef _WIN32
#include "./../component/clog.h"

//初始化事件
void CWorkerClientImplIOCP::Init()
{
	//创建IOCP
	iocp_.Create();
	CAbstractWorkerClient::Init();
}

//连接成功事件
void CWorkerClientImplIOCP::OnConnected()
{
	//关联本客户端socket
	iocp_.Reg(sockfd_);
}

//处理事件(使用iocp或者epoll的方式来重载)
void CWorkerClientImplIOCP::ProcessEvents(int wait_msec)
{
	if (!PostIocpEvents()) return;
	// 循环处理事件直到没有事件发生为止
	while (true)
	{
		int ret = ProcessIO(wait_msec);
		if (ret < 0)
		{
			// IOCP出错
			return;
		}
		else if (ret == 0)
		{
			// 没有事件发生
			break;
		}
	}
}

//投递投递的所有事件(在这里一般是发送和接受数据)
bool CWorkerClientImplIOCP::PostIocpEvents()
{
	if (NeedWrite())
	{
		auto pIoData = MakeSendIOData();
		if (pIoData)
		{
			if (!iocp_.PostSend(pIoData))
			{
				Close();
				return false;
			}
		}

		pIoData = MakeRecvIOData();
		if (pIoData)
		{
			if (!iocp_.PostRecv(pIoData))
			{
				Close();
				return false;
			}
		}
	}
	else
	{
		auto pIoData = MakeRecvIOData();
		if (pIoData)
		{
			if (!iocp_.PostRecv(pIoData))
			{
				Close();
				return false;
			}
		}
	}
	return true;
}

//处理io事件
int CWorkerClientImplIOCP::ProcessIO(int wait_msec)
{
	int ret = iocp_.Wait(io_event_, wait_msec);
	do
	{
		if (ret < 0)
		{
			//IOCP出错
			LOG_ERROR("CAbstractWorkerClient::Main error\n");
			break;
		}
		else if (ret == 0)
		{
			//没有事件
			break;
		}

		if (IO_TYPE::RECV == io_event_.pIOData->iotype)
		{//接收到了客户端数据
			if (io_event_.bytesTrans <= 0)
			{//远端断开
				LOG_DEBUG("RECV close sockfd=%d, bytesTrans=%d\n", (int)io_event_.pIOData->sockfd, io_event_.bytesTrans);
				Close();
				break;
			}
			//告诉接收数据的客户端本次接收到了多少数据，用于接收缓冲区的数据位置的偏移
			Recv4Iocp(io_event_.bytesTrans);
		}
		else if (IO_TYPE::SEND == io_event_.pIOData->iotype)
		{//已经把数据发送出去了
			if (io_event_.bytesTrans <= 0)
			{//远端断开
				LOG_DEBUG("SEND close sockfd=%d, bytesTrans=%d\n", (int)io_event_.pIOData->sockfd, io_event_.bytesTrans);
				Close();
				break;
			}
			//通知发送缓冲区发送了多少数据
			Send2Iocp(io_event_.bytesTrans);
		}
		else
		{
			LOG_WARNING("IOCP get a unknow type event\n");
			Close();
		}
	} while (0);
	return ret;
}

#endif
