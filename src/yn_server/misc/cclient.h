#ifndef __CCLIENT_H__
#define __CCLIENT_H__
#include "./../net/ciocp.h"
#include "./../export_api.h"
#include "cabstract_client.h"
#include <ctime>

/**
 * @brief 用在服务端中的客户端, 要处理心跳超时事件
 */
class COMMONLIB_EXPORT CClient : public CAbstractClient
{
public:
	//本客户端的编号，测试用
	int test_id = 0;

	//构造
	CClient(int sockfd = -1, 
		int send_buf_size = 4096, int recv_bu_size = 4096,
		int heart_check_timeout = 10000);

	//初始化事件
	virtual void Init() override;

	//把心跳计时置0
	void ResetDtHeart();

	//心跳检测
	bool CheckHeart(unsigned int dt);
private:
	//死亡心跳计时
	unsigned int dt_heart_ = 0;
	//死亡心跳计时上限
	unsigned int heart_check_timeout_ = 0;
};

#endif
