#ifndef __CWORKER_CLIENT_IMPL_EPOLL_H__
#define __CWORKER_CLIENT_IMPL_EPOLL_H__
#if __linux__
#include "cabstract_worker_client.h"
#include "./../export_api.h"
#include "./../net/cepoll.h"

/**
 * @brief 客户端使用epoll的收发数据方式的实现
 */
class CWorkerClientImplEpoll : public CAbstractWorkerClient
{
public:
	//初始化事件
	virtual void Init() override;

	//连接成功事件
	virtual void OnConnected() override;

	//处理事件(使用iocp或者epoll的方式来重载)
	virtual void ProcessEvents(int wait_msec) override;

	//根据client选择挂在节点的事件
	void PostIO();
private:
	CEpoll epoll_;
};

#endif

#endif
