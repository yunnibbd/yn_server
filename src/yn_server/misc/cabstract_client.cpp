#include "cabstract_client.h"
#include "./../component/clog.h"
#include "./../utils/ctools.h"
#include "./../net/cbuffer.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstddef>

#define MAX_SEND_PKG 2048

/**
 * @brief 构造
 * @param sockfd 本客户端持有的套接字
 * @param send_buf_size 本客户端发送缓冲区大小
 * @param recv_buf_size 本客户端发送缓冲区大小
 * @return
 */
CAbstractClient::CAbstractClient(int sockfd, int send_buf_size, int recv_buf_size)
	: sockfd_(sockfd), send_buf_size_(send_buf_size), recv_buf_size_(recv_buf_size)
{

}

/**
 * @brief 析构
 * @param
 * @return
 */
CAbstractClient::~CAbstractClient()
{
	is_init_ = false;
	Destory();
}

/**
 * @brief 获得本客户端的socket
 * @param
 * @return int 本客户端持有的套接字
 */
int CAbstractClient::sockfd()
{
	return sockfd_;
}

/**
 * @brief 初始化事件
 * @param
 * @return
 */
void CAbstractClient::Init()
{
	p_send_buf_ = new CBuffer(send_buf_size_);
	p_recv_buf_ = new CBuffer(recv_buf_size_);
	is_init_ = true;
}

/**
 * @brief 销毁资源
 * @param
 * @return
 */
void CAbstractClient::Destory()
{
	if (sockfd_ != -1)
	{
		CTools::CloseSocket(sockfd_);
		sockfd_ = -1;
	}
	if (p_send_buf_)
	{
		delete p_send_buf_;
		p_send_buf_ = nullptr;
	}
	if (p_recv_buf_)
	{
		delete p_recv_buf_;
		p_recv_buf_ = nullptr;
	}
}

/**
 * @brief 只关闭socket
 * @param
 * @return
 */
void CAbstractClient::CloseSocket()
{
	if (sockfd_ != -1)
	{
		CTools::CloseSocket(sockfd_);
		sockfd_ = -1;
	}
}

/**
 * @brief 使用websocket的方式发送数据
 * @param data 数据源
 * @param data_len 数据源长度
 * @return
 */
int CAbstractClient::WSSendData(char *data, int data_len)
{
	char *send_buffer = nullptr;

	int header = 1; //0x81
	if (data_len <= 125)
		header++;
	else if (data_len <= 0xffff)
		header += 3;
	else
		header += 9;
	if (header + data_len > MAX_SEND_PKG)
		send_buffer = new char[header + data_len];
	else
		send_buffer = new char[MAX_SEND_PKG];

	unsigned int send_len;
	//固定的头
	send_buffer[0] = 0x81;
	if (data_len <= 125)
	{
		//最高bit为0
		send_buffer[1] = data_len;
		send_len = 2;
	}
	else if (data_len <= 0xffff)
	{
		send_buffer[1] = 126;
		send_buffer[2] = (data_len & 0x000000ff);
		send_buffer[3] = ((data_len & 0x0000ff00) >> 8);
		send_len = 4;
	}
	else
	{
		send_buffer[2] = (data_len & 0x000000ff);
		send_buffer[3] = ((data_len & 0x0000ff00) >> 8);
		send_buffer[4] = ((data_len & 0x00ff0000) >> 16);
		send_buffer[5] = ((data_len & 0xff000000) >> 24);

		send_buffer[6] = 0;
		send_buffer[7] = 0;
		send_buffer[8] = 0;
		send_buffer[9] = 0;
		send_len = 10;
	}
	memcpy(send_buffer + send_len, data, data_len);
	send_len += data_len;
	int ret = SendData(send_buffer, send_len);

	delete[] send_buffer;
	return ret;
}

/**
 * @brief 发回数据
 * @param data 数据源
 * @param data_len 数据源长度
 * @return
 */
int CAbstractClient::SendData(const char *data, int data_len)
{
	if (p_send_buf_->Push(data, data_len))
		//只是将数据放入发送缓冲区，在下次检测可以发送的时候再发送
		return data_len;
	return 0;
}

/**
 * @brief 立即发送发送缓冲区的数据
 * @param
 * @return int 本次发送的长度
 */
int CAbstractClient::SendDataReal()
{
	return p_send_buf_->Write2Socket(sockfd_);
}

/**
 * @brief 接收数据
 * @param
 * @return
 */
int CAbstractClient::RecvData()
{
	return p_recv_buf_->Read4Socket(sockfd_);
}

/**
 * @brief 发送缓冲区是否有数据需要发送
 * @param
 * @return
 */
bool CAbstractClient::NeedWrite()
{
	return p_send_buf_->HasData();
}

/**
 * @brief 接收缓冲区是否有数据需要处理
 * @param
 * @return
 */
bool CAbstractClient::NeedRead()
{
	return p_recv_buf_->HasData();
}

#ifdef _WIN32
/**
 * @brief 用套接字和缓冲区制作一个用于iocp接收数据的缓冲区
 * @param
 * @return
 */
IO_DATA_BASE *CAbstractClient::MakeRecvIOData()
{
	if (is_post_recv_)
		return nullptr;
	is_post_recv_ = true;
	return p_recv_buf_->MakeRecvIOData(sockfd_);
}

/**
 * @brief 告知缓冲区本次接收到数据的长度
 * @param nRecv 本次接收的数据长度
 * @return
 */
void CAbstractClient::Recv4Iocp(int nRecv)
{
	if (!is_post_recv_)
		LOG_DEBUG("CAbstractClient recv4Iocp, is_post_recv_=false");
	is_post_recv_ = false;
	p_recv_buf_->Read4Iocp(nRecv);
}

/**
 * @brief 用套接字和缓冲区制作一个用于iocp发送数据的缓冲区
 * @param
 * @return
 */
IO_DATA_BASE *CAbstractClient::MakeSendIOData()
{
	if (is_post_send_)
		return nullptr;
	is_post_send_ = true;
	return p_send_buf_->MakeSendIOData(sockfd_);
}

/**
 * @brief 告知本次要发送的数据长度
 * @param nSend 本次发送数据的长度
 * @return
 */
void CAbstractClient::Send2Iocp(int nSend)
{
	if (!is_post_send_)
		LOG_DEBUG("CAbstractClient send2Iocp, is_post_send_=false");
	is_post_send_ = false;
	p_send_buf_->Write2Iocp(nSend);
}

/**
 * @brief 是否投递了接收或者发送数据的任务
 * @param
 * @return
 */
bool CAbstractClient::IsPostIoAction()
{
	return is_post_recv_ || is_post_send_;
}
#endif //#ifdef _WIN32
