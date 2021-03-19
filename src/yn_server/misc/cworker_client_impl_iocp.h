#ifndef __CWORKER_CLIENT_IMPL_IOCP_H__
#define __CWORKER_CLIENT_IMPL_IOCP_H__
#ifdef _WIN32
#include "cabstract_worker_client.h"
#include "./../net/ciocp.h"
#include "./../export_api.h"

/**
 * @brief 客户端使用iocp的收发数据方式的实现
 */
class COMMONLIB_EXPORT CWorkerClientImplIOCP : public CAbstractWorkerClient
{
public:
	//初始化事件
	virtual void Init() override;

	//连接成功事件
	virtual void OnConnected() override;

	//处理事件(使用iocp或者epoll的方式来重载)
	virtual void ProcessEvents(int wait_msec) override;

	//投递投递的所有事件(在这里一般是发送和接受数据)
	bool PostIocpEvents();

	//处理io事件
	int ProcessIO(int wait_msec);
private:
#ifdef _WIN32
	//用于监听可发送数据的iocp
	CIOCP iocp_;
	//描述iocp的事件
	IO_EVENT io_event_;
#endif
};

#endif

#endif
