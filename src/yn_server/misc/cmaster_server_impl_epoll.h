#ifndef __CMASTER_SERVER_IMPL_EPOLL_H__
#define __CMASTER_SERVER_IMPL_EPOLL_H__
#if __linux__
#include "cabstract_master_server.h"

/**
 * @brief 接收客户端的服务端使用epoll的实现
 */
class CMatserServerImplEpoll : public CAbstractMasterServer
{
public:
	//服务端线程入口函数
	virtual void Main(CThread *pthread) override;

	//创建多个消息处理服务端并启动(重载决定使用什么iocp, epoll)
	virtual void Start(int num) override;

	//接受客户端
	void Accept();
};

#endif

#endif
