#ifndef __CBUFFER_H__
#define __CBUFFER_H__
#include <string.h>
#include "ciocp.h"
#include "./../component/cobject_pool.hpp"
#include "./../export_api.h"

/**
 * @brief 对网络数据缓冲区的封装
 */
class COMMONLIB_EXPORT CBuffer : public CObjectPool<CBuffer, 4096>
{
public:
	CBuffer(int size = 8192);

	~CBuffer();

	//往缓冲区例放数据
	bool Push(const char *data, int data_len);

	//将缓冲区一部分数据移除
	void Pop(int len);

	//从socket中读取数据
	int Read4Socket(int sockfd);

	//往socket中写数据
	int Write2Socket(int sockfd);

#ifdef _WIN32
	//所有针对iocp的数据存储和读取
	IO_DATA_BASE *MakeRecvIOData(int sockfd);

	// 数据对于IOCP来说是接收完成了告诉程序，那么就需要告诉缓冲区本次接收到了多少长度
	bool Read4Iocp(int nRecv);

	IO_DATA_BASE* MakeSendIOData(int sockfd);

	bool Write2Iocp(int nSend);
#endif

	//缓冲区里是否有数据
	bool HasData();

	//得到缓冲区指针
	char *Data();

	//返回buffer总大小
	int BuffSize();

	//返回已有数据的长度
	int DataLen();
private:
	// 缓冲区指针
	char *p_buffer_ = nullptr;
	// 缓冲区的数据尾部位置(已有数据大小)
	int n_last_ = 0;
	// 缓冲区总的空间大小(字节长度)
	int n_size_ = 0;
	// 缓冲区写满计数
	int full_count_ = 0;
#ifdef _WIN32
	//IOCP使用的数据上下文
	IO_DATA_BASE io_data_ = { 0 };
#endif
};

#endif
