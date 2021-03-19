#include "ctools.h"
#include <chrono>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif
using namespace std::chrono;

//设置端口可复用
bool CTools::MakeReuseAddr(int socket)
{
	int flag = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag)))
		return false;
	return true;
}

//关闭一个socket
void CTools::CloseSocket(int socket)
{
#ifdef _WIN32
	closesocket(socket);
#else
	close(socket);
#endif
}

unsigned int CTools::GetCurMs()
{
#ifdef _WIN32
	return GetTickCount();
#else
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);

	return ((tv.tv_usec / 1000) + tv.tv_sec * 1000);
#endif
}
