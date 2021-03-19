#include "cworker_server_impl_epoll.h"
#if __linux__
#include "cclient.h"
#include "cabstract_master_server.h"
#include "./../component/clog.h"

void CWorkerServerImplEpoll::Init()
{
	CAbstractWorkerServer::Init();
	//初始化最多挂在10240个事件节点
	epoll_.Create(10240);
}

//客户端加入事件
void CWorkerServerImplEpoll::OnJoin(CClient *pclient)
{
	//客户端加入监听读事件
	if (epoll_.Ctl(EPOLL_CTL_ADD, pclient, EPOLLIN) == -1)
	{
		//挂在到epoll节点上失败
		if (master_server_)
			master_server_->OnLeave(pclient);
		delete pclient;
		pclient = nullptr;
		return;
	}
}

//处理所有要发生的事件
void CWorkerServerImplEpoll::ProcessEvents()
{
	//检测需要发送给客户端数据的客户端(这个可以根据情况而改变)
	for (auto &pclient : clients_)
	{
		if (pclient->NeedWrite())
			//如果客户端有数据可发送，那么就检测可写
			epoll_.Ctl(EPOLL_CTL_MOD, pclient, EPOLLOUT | EPOLLIN);
		else
			//如果没有数据要发送, 那就检测可读
			epoll_.Ctl(EPOLL_CTL_MOD, pclient, EPOLLIN);
	}
	//处理定时器, 返回本次要休眠的时间
	int sleep_msec = timer_.ProcessTimer();
	if (sleep_msec <= 0)
		sleep_msec = -1;
	int ret = epoll_.Wait(sleep_msec);
	do {
		if (ret < 0)
		{
			LOG_WARNING("CWorkerServerImplEpoll::ProcessEvents Wait failed\n");
			break;
		}
		else if (ret == 0)
		{
			//没有事件发生
			return;
		}

		auto events = epoll_.events();
		for (int i = 0; i < ret; ++i)
		{
			CClient *pclient = (CClient*)events[i].data.ptr;
			if (pclient)
			{
				//客户端可读
				if (events[i].events & EPOLLIN)
				{
					// 接受客户端数据
					if (pclient->RecvData() <= 0)
					{
						LOG_DEBUG("RECV close sockfd=%d\n", pclient->sockfd());
						clients_.erase(pclient);
						//触发客户端离开事件
						OnLeave(pclient);
						continue;
					}
					//触发接收事件
					OnRecv(pclient);
				}

				//客户端可写
				if (events[i].events & EPOLLOUT)
				{
					if (pclient->SendDataReal() <= 0)
					{
						LOG_DEBUG("SEND close sockfd=%d\n", pclient->sockfd());
						clients_.erase(pclient);
						//触发客户端离开事件
						OnLeave(pclient);
					}
				}
			}
		}
	} while (0);
}

#endif
