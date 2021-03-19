#ifndef __CABSTRACT_CLIENT_H__
#define __CABSTRACT_CLIENT_H__
#include "./../export_api.h"
#include "./../net/ciocp.h"
class CBuffer;

enum SocketType
{
	TCP_SOCKET_IO = 0,  //tcp socket
	WEB_SOCKET_IO = 1,  //websocket
};

enum ProtocalType
{
	PROTO_PROTOCAL = 1,  //不解析，收到什么数据直接调用ReadCB
	JSON_PROTOCAL = 2,   //json协议
};

/**
 * @brief client的基类, 其中只封装数据IO操作
 */
class COMMONLIB_EXPORT CAbstractClient
{
public:
	/**
	 * @brief 构造
	 * @param sockfd 本客户端持有的套接字
	 * @param send_buf_size 本客户端发送缓冲区大小
	 * @param recv_buf_size 本客户端发送缓冲区大小
	 * @return
	 */
	CAbstractClient(int sockfd = -1, 
		int send_buf_size = 4096, int recv_buf_size = 4096);

	/**
	 * @brief 析构
	 * @param
	 * @return
	 */
	~CAbstractClient();

	/**
	 * @brief 初始化事件
	 * @param
	 * @return
	 */
	virtual void Init();

	/**
	 * @brief 连接成功事件
	 * @param
	 * @return
	 */
	virtual void OnConnected() {}

	/**
	 * @brief 销毁资源
	 * @param
	 * @return
	 */
	void Destory();

	/**
	 * @brief 获得本客户端的socket
	 * @param
	 * @return int 本客户端持有的套接字
	 */
	int sockfd();

	/**
	 * @brief 只关闭socket
	 * @param
	 * @return
	 */
	void CloseSocket();

	/**
	 * @brief 发回数据
	 * @param data 数据源
	 * @param data_len 数据源长度
	 * @return
	 */
	int SendData(const char *data, int data_len);

	/**
	 * @brief 使用websocket的方式发送数据
	 * @param data 数据源
	 * @param data_len 数据源长度
	 * @return
	 */
	int WSSendData(char *data, int data_len);

	/**
	 * @brief 立即发送发送缓冲区的数据
	 * @param
	 * @return int 本次发送的长度
	 */
	int SendDataReal();

	/**
	 * @brief 接收数据
	 * @param
	 * @return
	 */
	int RecvData();

	/**
	 * @brief 发送缓冲区是否有数据需要发送
	 * @param
	 * @return
	 */
	bool NeedWrite();

	/**
	 * @brief 接收缓冲区是否有数据需要处理
	 * @param
	 * @return
	 */
	bool NeedRead();

	//获得接收缓冲区指针
	CBuffer *p_recv_buf() { return p_recv_buf_; }

	SocketType GetSocketType() { return socket_type_;  }
	void SetSocketType(SocketType type) { socket_type_ = type; }

	ProtocalType GetProtocalType() { return protocal_type_; }
	void SetProtocalType(ProtocalType type) { protocal_type_ = type; }

#ifdef _WIN32
	/**
	 * @brief 用套接字和缓冲区制作一个用于iocp接收数据的缓冲区
	 * @param
	 * @return
	 */
	IO_DATA_BASE *MakeRecvIOData();

	/**
	 * @brief 告知缓冲区本次接收到数据的长度
	 * @param nRecv 本次接收的数据长度
	 * @return
	 */
	void Recv4Iocp(int nRecv);

	/**
	 * @brief 用套接字和缓冲区制作一个用于iocp发送数据的缓冲区
	 * @param
	 * @return
	 */
	IO_DATA_BASE *MakeSendIOData();

	/**
	 * @brief 告知本次要发送的数据长度
	 * @param nSend 本次发送数据的长度
	 * @return
	 */
	void Send2Iocp(int nSend);

	/**
	 * @brief 是否投递了接收或者发送数据的任务
	 * @param
	 * @return
	 */
	bool IsPostIoAction();
#endif //#ifdef _WIN32

	//是否初始化标志位
	bool is_init_ = false;
	//本客户端对应的远端客户端的ip
	char ip_[32] = { 0 };
	//本客户端对应的远端客户端的端口
	unsigned short port_ = 0;
	//SocketType类型为WEB_SOCKET_IO下时候先进行了连接
	bool is_ws_connected = false;
protected:
	//接收缓冲区指针
	CBuffer *p_recv_buf_ = nullptr;
	//发送缓冲区指针
	CBuffer *p_send_buf_ = nullptr;
	//接受缓冲区大小
	int send_buf_size_ = 0;
	//发送缓冲区大小
	int recv_buf_size_ = 0;
	//本客户端持有的socket
	int sockfd_ = -1;
	//0 表示TCP socket, 1表示是 websocket
	SocketType socket_type_ = TCP_SOCKET_IO;
	ProtocalType protocal_type_ = JSON_PROTOCAL;
#ifdef _WIN32
	//IOCP是否投递了请求
	bool is_post_recv_ = false;
	bool is_post_send_ = false;
#endif
};

#endif
