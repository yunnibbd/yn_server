#ifndef __CWORKER_SERVER_IMPL_EPOLL_H__
#define __CWORKER_SERVER_IMPL_EPOLL_H__
#if __linux__
#include "cabstract_worker_server.h"
#include "./../net/cepoll.h"

class CClient;

/**
 * @brief 消息处理服务端使用epoll的实现
 */
class CWorkerServerImplEpoll : public CAbstractWorkerServer
{
public:
	//初始化事件
	virtual void Init() override;

	//客户端加入事件
	void OnJoin(CClient *pclient) override;

	//处理所有要发生的事件
	virtual void ProcessEvents() override;

private:
	CEpoll epoll_;
};

#endif

#endif
