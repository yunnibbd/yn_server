#ifndef __CABSTRACT_WORKER_SERVER_IMPL_IOCP_H__
#define __CABSTRACT_WORKER_SERVER_IMPL_IOCP_H__
#ifdef _WIN32
#include "cabstract_worker_server.h"
#include "./../net/ciocp.h"

class CClient;

/**
 * @brief 消息处理服务端使用IOCP的实现
  */
class CWorkerServerImplIocp : public CAbstractWorkerServer
{
public:
	//初始化事件
	virtual void Init() override;

	//客户端加入事件
	void OnJoin(CClient *pclient) override;

	//处理所有要发生的事件
	virtual void ProcessEvents() override;

	//让IOCP把要投递的所有事件投递出去
	void PostIocpEvents();

	//处理io事件
	int ProcessIO();

private:
	//IOCP对象
	CIOCP iocp_;
	//模仿epool的event
	IO_EVENT io_event_;
	//用于iocp事件的数据指针
	IO_DATA_BASE io_data_ = { 0 };
};

#endif

#endif
