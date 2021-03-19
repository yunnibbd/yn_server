#include "cclient.h"
#include "./../component/clog.h"
#include "./../component/cconfig.h"
#include "./../net/cbuffer.h"

//构造
CClient::CClient(int sockfd, 
				 int send_buf_size, int recv_bu_size, 
				 int heart_check_timeout
				)
	: CAbstractClient(sockfd, send_buf_size, recv_bu_size), 
	heart_check_timeout_(heart_check_timeout)
{
	//本类用做服务端的队列客户端, 由服务端创建
	Init();
}

void CClient::Init()
{
	//初始化client基类
	CAbstractClient::Init();
	CConfig *p_config = CConfig::Get();
}

//把心跳计时置0
void CClient::ResetDtHeart()
{
	dt_heart_ = 0;
}

//心跳检测
bool CClient::CheckHeart(unsigned int dt)
{
	dt_heart_ += dt;
	if (dt_heart_ >= heart_check_timeout_)
	{
		//LOG_DEBUG("chackHeart dead: %d, time: %d", sockfd_, dt_heart_);
		return true;
	}
	return false;
}
