#include "enable_mem_pool.h" //重载new delete使用内存池
#include "cconfig.h"
#include "cmaster_server.h"
#include "cworker_client.h"
#include <iostream>
#include <vector>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#include <signal.h>
#pragma comment(lib, "ws2_32.lib")
#endif
using namespace std;

class MyServer : public CMasterServer
{
public:
	//处理json回调(设置使用JSON传输数据时候会自动解析调用)
	//开启的消息服务是单个是线程安全的，多个是线程不安全的
	virtual void OnJson(CClient *pclient, char *data, int data_len) override
	{
		data[data_len] = 0;
		cout << data << endl;
	}

	//收到数据回调(设置使用PROTO_PROTOCAL直接将数据传入此函数)
	//开启的消息服务是单个是线程安全的，多个是线程不安全的
	virtual void ReadCB(CClient *pclient, char *data, int data_len) override
	{
		data[data_len] = 0;
		cout << "recv from client:" << data << endl;
		pclient->SendData(data, data_len);
	}
};

class MyClient : public CWorkerClient
{
public:
	virtual void OnConnected() override
	{
		CWorkerClient::OnConnected();
		cout << "连接成功" << endl;
		this->SendData("OK", 3);
	}

	virtual void ReadCB(char *data, int data_len) override
	{
		data[data_len] = 0;
		cout << "recv from server:" << data << endl;
		this->SendData(data, data_len);
	}
};

void start_tcp_server(const char *server_ip, unsigned short server_port)
{
	//创建server
	MyServer *server = new MyServer;
	//设置server port
	server->set_server_port(server_port);
	//设置server ip
	server->set_server_ip(server_ip);
	//初始化server
	server->Init();
	//设置通信方式
	//server->set_socket_type(WEB_SOCKET_IO);
	//server->set_protocol_type(JSON_PROTOCAL);
	server->set_socket_type(TCP_SOCKET_IO);
	server->set_protocol_type(PROTO_PROTOCAL);
	//开启2个处理消息收发的server
	server->Start(2);
}

void start_websocket_server(const char *server_ip, unsigned short server_port)
{
	//创建server
	MyServer *websocket_server = new MyServer;
	//设置server port
	websocket_server->set_server_port(server_port);
	//设置server ip
	websocket_server->set_server_ip(server_ip);
	//初始化server
	websocket_server->Init();
	//设置通信方式
	websocket_server->set_socket_type(WEB_SOCKET_IO);
	websocket_server->set_protocol_type(JSON_PROTOCAL);
	//websocket_server->set_socket_type(TCP_SOCKET_IO);
	//websocket_server->set_protocol_type(PROTO_PROTOCAL);
	//开启2个处理消息收发的server
	websocket_server->Start(2);
}

void start_tcp_client(const char *server_ip, unsigned short server_port)
{
	MyClient client;
	//设置客户端要连接的服务器ip
	client.set_server_ip(server_ip);
	//设置客户端要连接的服务器端口
	client.set_server_port(server_port);
	//初始化客户端
	client.Init();

	//开始连接，连接成功后调用连接回调
	client.StartConnect();

	while (client.IsConnecting())
	{
		//Main函数处理收发数据
		client.Main();
		CThread::Sleep(1000);
	}
}

int main()
{
#ifdef _WIN32
	WSAData wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif
	//初始化配置文件
	CConfig *p_config = CConfig::Get();
	p_config->Load("yn.conf");
	//获得要绑定的ip和端口
	char *server_ip = p_config->GetString("BindnIP");
	if (!server_ip) 
		server_ip = (char*)"127.0.0.1";
	int server_port = p_config->GetIntDefault("ListenPort", 8000);

	start_websocket_server(server_ip, server_port);

	while (true)
		CThread::Sleep(10000);
	return 0;
}
