#ifndef __CABSTRACT_MASTER_SERVER_H__
#define __CABSTRACT_MASTER_SERVER_H__
#include "cabstract_server.h"
#include "cabstract_client.h"
#include "./../export_api.h"
#include "./../component/cthread.h"
#include <atomic>
#include <vector>

class CClient;
class CAbstractWorkerServer;

/**
 * @brief 接收客户端的服务端的抽象类
 */
class COMMONLIB_EXPORT CAbstractMasterServer : public CAbstractServer
{
public:
	//初始化事件
	virtual void Init() override;

	//客户端加入事件
	virtual void OnJoin(CClient *pclient) override;

	//客户端离开事件
	virtual void OnLeave(CClient *pclient) override;

	//服务端线程入口函数
	virtual void Main(CThread *pthread) = 0;

	//创建多个消息处理服务端并启动(重载决定使用什么iocp, epoll)
	virtual void Start(int num) = 0;

	//初始化socket
	void InitSocket();

	//决定本服务使用什么方式来进行通信
	void set_socket_type(SocketType type = WEB_SOCKET_IO) 
	{
		socket_type_ = type;
	}

	//决定本服务使用什么协议来进行通信
	void set_protocol_type(ProtocalType type = JSON_PROTOCAL)
	{
		protocal_type_ = type;
	}
protected:
	//客户端发送缓冲区大小
	int send_buf_size_ = 0;
	//客户端接收缓冲区大小
	int recv_buf_size_ = 0;
	//客户端心跳检测超时时间
	int heart_check_timeout_ = 0;
	//本服务端最多接受多少客户端
	int max_client_num_ = 0;
	//客户端socket类型
	SocketType socket_type_ = WEB_SOCKET_IO;
	//客户端传输数据协议类型
	ProtocalType protocal_type_ = JSON_PROTOCAL;
	//本服务端现在有多少客户端
	std::atomic<int> client_num_{0};
	//存放所有消息服务端的指针
	std::vector<CAbstractWorkerServer*> all_cmsg_server_;

private:
	//本类线程对象
	CThread thread_;
};

#endif
