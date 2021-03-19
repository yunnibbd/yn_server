#ifndef __CMASTER_SERVER_IMPL_IOCP_H__
#define __CMASTER_SERVER_IMPL_IOCP_H__
#ifdef _WIN32
#include "cabstract_master_server.h"
#include "./../net/ciocp.h"
#include "./../export_api.h"

/**
 * @brief 接收客户端的服务端使用iocp的实现
 */
class COMMONLIB_EXPORT CMasterServerImplIOCP : public CAbstractMasterServer
{
public:
	//服务端线程入口函数
	virtual void Main(CThread *pthread) override;

	//创建多个消息处理服务端并启动(重载决定使用什么iocp, epoll)
	virtual void Start(int num) override;

	//IOCP的方式接收客户端
	bool IocpAccept(int cSock);
private:
	//IOCP对象
	CIOCP iocp_;
	//模仿epoll的event
	IO_EVENT io_event_;
	//用于接收新客户端的数据指针
	IO_DATA_BASE io_data_ = { 0 };
	//用于接收新客户端的缓冲区
	char buffer_[512] = { 0 };
};

#endif

#endif
