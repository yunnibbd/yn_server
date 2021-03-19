#include "cworker_client_impl_epoll.h"
#if __linux__
#include "./../component/clog.h"

//初始化事件
void CWorkerClientImplEpoll::Init()
{
	epoll_.Create(1);
	CAbstractWorkerClient::Init();
}

//连接成功事件
void CWorkerClientImplEpoll::OnConnected()
{
	
}

//处理事件(使用iocp或者epoll的方式来重载)
void CWorkerClientImplEpoll::ProcessEvents(int wait_msec)
{
	PostIO();
	int ret = epoll_.Wait(wait_msec);
	do {
		if (ret < 0)
		{
			LOG_WARNING("CWorkerClientImplEpoll::ProcessEvents Wait failed\n");
			break;
		}
		else if (ret == 0)
		{
			//没有事件发生
			return;
		}

		auto fd_event = epoll_.events()[0];
		int sockfd = fd_event.data.fd;
		//可读
		if (fd_event.events & EPOLLIN)
		{
			//接收数据
			if (RecvData() <= 0)
			{
				LOG_DEBUG("RECV close sockfd=%d\n", sockfd_);
				Close();
				break;
			}
		}

		//可写
		if (fd_event.events & EPOLLOUT)
		{
			if (SendDataReal() <= 0)
			{
				LOG_DEBUG("SEND close sockfd=%d\n", sockfd_);
				Close();
				break;
			}
		}
	} while (0);
}

//根据client选择挂在节点的事件
void CWorkerClientImplEpoll::PostIO()
{
	if (NeedWrite())
		epoll_.Ctl(EPOLL_CTL_MOD, sockfd_, EPOLLOUT | EPOLLIN);
	else
		epoll_.Ctl(EPOLL_CTL_MOD, sockfd_, EPOLLIN);
}

#endif
