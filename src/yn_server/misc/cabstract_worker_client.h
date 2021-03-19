#ifndef __CABSTRACT_WORKER_CLIENT_H__
#define __CABSTRACT_WORKER_CLIENT_H__
#include "cabstract_client.h"
#include "cclient.h"
#include "./../net/cbuffer.h"
#include "./../net/ctimer.h"
#include "./../export_api.h"

//发送消息的客户端
class COMMONLIB_EXPORT CAbstractWorkerClient : public CAbstractClient
{
public:
	//初始化事件
	virtual void Init() override;

	//连接成功事件
	virtual void OnConnected() {}

	//收到数据回调
	virtual void ReadCB(char* data, int data_len) {}

	//关闭本客户端
	void Close();
	
	//初始化socket
	void InitSocket();

	//此函数做io处理, 多次调用
	bool Main(int wait_msec = 1);

	//处理io事件(使用iocp或者epoll的方式来重载)
	virtual void ProcessEvents(int wait_msec) = 0;

	//处理收到的消息
	void ProcessMsg();
	
	//开始连接服务端
	bool StartConnect();

	//检查连接的执行函数, 如果断开连接就开始重连接
	void CheckConnect();

	//设置要连接的服务端ip
	void set_server_ip(const char *ip);

	//设置要连接的服务端port
	void set_server_port(unsigned short port) { server_port_ = port; }

	//是否与服务端保持连接
	bool IsConnecting() { return is_connected_; }

protected:
	//要连接的服务端地址
	char server_ip_[16] = { 0 };
	//要连接的服务端端口
	unsigned short server_port_ = 0;
	//是否与服务端建立了连接
	bool is_connected_ = false;
	//定时器对象(用于定时检查连接)
	CTimer timer_;
};

#endif
