#ifndef __CABSTRACT_WORKER_SERVER_H__
#define __CABSTRACT_WORKER_SERVER_H__
#include "cabstract_server.h"
#include "./../component/cthread.h"
#include <set>
#include <mutex>

class CClient;
class CAbstractMasterServer;

/**
 * @brief 消息处理服务端的抽象类
 */
class CAbstractWorkerServer : public CAbstractServer
{
public:
	//初始化事件
	virtual void Init() override;

	//处理json回调
	virtual void OnJson(CClient *pclient, char *data, int data_len) override;

	//处理收到数据的回调
	virtual void ReadCB(CClient *pclient, char *data, int data_len) override;

	//客户端离开事件
	virtual void OnLeave(CClient *pclient) override;

	//客户端加入事件
	virtual void OnJoin(CClient *pclient) = 0;

	//服务端接收消息事件
	virtual void OnRecv(CClient *pclient) override;

	//开启消息处理服务端线程
	void Start();

	//消息服务端线程入口函数
	void Main(CThread *pthread);

	//处理所有要发生的事件
	virtual void ProcessEvents() = 0;

	//心跳检测所有客户端
	void CheckHeart();

	//处理消息
	void ProcessMsg();

	//使用json方式进行解析
	bool OnJsonProtocalRecv(CClient *pclient);

	//本websocket客户端进行连接
	bool StartWSConnect(CClient *pclient);

	//本websocket客户端收到的信息
	bool OnWsPackRecv(CClient *pclient);
	
	//解析ws数据,根据协议调用对应的处理回调函数
	void ParserWsPack(CClient *pclient, char* body, int body_len, char* mask, int protocal_type);

	//此函数由master服务端调用, 将新接收的客户端加入本消息服务端的客户端缓冲队列
	void AddClientToClientsBuffer(CClient *pclient);

	//获取本消息处理服务端的所有客户端数量
	int AllClientNum();

	//注册有限定时器
	unsigned int RegisterTimer(time_callback cb, void* udata, unsigned int after_msec);

	//注册无限次定时器
	unsigned int RegisterSchedule(time_callback cb, void* udata, unsigned int after_msec);

	//取消定时器
	void UnRegisterTimer(unsigned int timerid);

	void set_master_server(CAbstractMasterServer *p) { master_server_ = p; }

protected:
	//本消息处理服务端的Accept服务端(用于触发它的事件)
	CAbstractMasterServer *master_server_ = nullptr;
	//客户端缓冲队列, accept服务器将接收到的客户端放入这个缓冲队列中
	std::set<CClient*> clients_buffer_;
	//客户端缓冲区队列互斥量
	std::mutex clients_buffer_mutex_;
	//本服务端下接收的所有客户端
	std::set<CClient*> clients_;
	//定时器对象
	CTimer timer_;
	//是否开启心跳检测 1开启, 0不开启
	int is_check_heart_ = 1;
private:
	//本类线程对象
	CThread thread_;
};

#endif
