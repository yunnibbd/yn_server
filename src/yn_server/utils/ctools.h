#ifndef __CTOOLS_H__
#define __CTOOLS_H__
#include "./../export_api.h"
#include <time.h>

/**
 * @brief 工具类
 */
class COMMONLIB_EXPORT CTools
{
public:
	//设置端口可复用
	static bool MakeReuseAddr(int socket);

	//关闭一个socket
	static void CloseSocket(int socket);

	//获取当前时间戳(毫秒)
	static unsigned int GetCurMs();
};

#endif
