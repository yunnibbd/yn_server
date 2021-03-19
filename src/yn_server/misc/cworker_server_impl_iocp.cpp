#include "cworker_server_impl_iocp.h"
#ifdef _WIN32
#include "cabstract_master_server.h"
#include "cclient.h"
#include "./../component/clog.h"

void CWorkerServerImplIocp::Init()
{
	CAbstractWorkerServer::Init();
	//IOCP对象创建
	iocp_.Create();
}

//处理所有要发生的事件
void CWorkerServerImplIocp::ProcessEvents()
{
	//投递所有事件
	PostIocpEvents();

	//iocp一次只处理一个事件, 一直处理完上面投递的为止
	while (true)
	{
		//处理io事件
		int ret = ProcessIO();
		if (ret < 0)
			//IOCP出错
			break;
		else if (ret == 0)
			//没有事件
			break;
	}
}

//让IOCP把要投递的所有事件投递出去
void CWorkerServerImplIocp::PostIocpEvents()
{
	auto iter = clients_.begin();
	auto end = clients_.end();
	for (; iter != end;)
	{//遍历所有客户端向IOCP投递任务事件请求
		auto pclient = *iter;
		if (pclient->NeedWrite())
		{//如果当前客户端发送缓冲区中有数据(投递发送和接收数据的任务)
			auto pIoData = pclient->MakeSendIOData();
			if (pIoData)
			{
				if (!iocp_.PostSend(pIoData))
				{
					//远端关闭连接
					iter = clients_.erase(iter);
					OnLeave(pclient);
					continue;
				}
			}

			pIoData = pclient->MakeRecvIOData();
			if (pIoData)
			{
				if (!iocp_.PostRecv(pIoData))
				{
					/*
						远端关闭连接, 
						走到这里代表已经投递了一次发送事件请求,
						这里将其关闭的话, WSARecv会抛出10053
						并且还会触发一次send事件, 
						但是在这里已经删除了接收和发送缓冲区,
						就会访问非法内存,
						这里的解决方案是：
							在正式客户端中判断有无再访问
					*/
					iter = clients_.erase(iter);
					OnLeave(pclient);
					continue;
				}
			}
		}
		else
		{
			//没有数据要发送(只投递接受数据的任务)
			auto pIoData = pclient->MakeRecvIOData();
			if (pIoData)
			{
				if (!iocp_.PostRecv(pIoData))
				{
					//远端关闭连接
					iter = clients_.erase(iter);
					OnLeave(pclient);
					continue;
				}
			}
		}
		++iter;
	}
}

//处理io事件
int CWorkerServerImplIocp::ProcessIO()
{
	//本次IOCP的Wait函数休眠的时间
	int sleep_msec = timer_.ProcessTimer();
	if (sleep_msec <= 0)
		sleep_msec = INFINITE;
	int ret = iocp_.Wait(io_event_, sleep_msec);
	do
	{
		if (ret < 0)
			//IOCP出错
			//LOG_ERROR("CAbstractServer::Main\n");
			break;
		else if (ret == 0)
			//没有事件
			break;

		auto io_type = io_event_.pIOData->iotype;

		if (IO_TYPE::RECV == io_type)
		{//接收到了客户端数据
			auto pclient = (CClient*)io_event_.data.ptr;
			if (io_event_.bytesTrans <= 0)
			{//客户端断开
				LOG_DEBUG("RECV close sockfd=%d, bytesTrans=%d\n", (int)io_event_.pIOData->sockfd, io_event_.bytesTrans);
				clients_.erase(pclient);
				//触发客户端离开事件
				OnLeave(pclient);
				break;
			}
			//告诉接收数据的客户端本次接收到了多少数据，用于接收缓冲区的数据位置的偏移
			pclient->Recv4Iocp(io_event_.bytesTrans);
			//接收消息计数
			OnRecv(pclient);
		}
		else if (IO_TYPE::SEND == io_type)
		{//已经把数据发送出去了
			auto pclient = (CClient*)io_event_.data.ptr;
			if (io_event_.bytesTrans <= 0)
			{//客户端断开
				LOG_DEBUG("SEND close sockfd=%d, bytesTrans=%d\n", (int)io_event_.pIOData->sockfd, io_event_.bytesTrans);
				clients_.erase(pclient);
				//触发客户端离开事件
				OnLeave(pclient);
				break;
			}
			//通知发送缓冲区发送了多少数据
			if (clients_.find(pclient) != clients_.end())
				pclient->Send2Iocp(io_event_.bytesTrans);
		}
		else
		{
			auto pclient = (CClient*)io_event_.data.ptr;
			LOG_INFO("IOCP get a unknow type event \n");
			clients_.erase(pclient);
			//触发客户端离开事件
			OnLeave(pclient);
		}
	} while (0);
	return ret;
}

//客户端加入事件
void CWorkerServerImplIocp::OnJoin(CClient *pclient)
{
	//注册关联客户端
	if (!iocp_.Reg(pclient->sockfd(), pclient))
	{
		//关联失败
		if (master_server_)
			master_server_->OnLeave(pclient);
		delete pclient;
		pclient = nullptr;
		return;
	}
}

#endif
