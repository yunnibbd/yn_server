#ifndef __CWORKER_CLIENT_H__
#define __CWORKER_CLIENT_H__
#ifdef _WIN32
#include "./misc/cworker_client_impl_iocp.h"
#else
#include "./misc/worker_client_impl_epoll.h"
#endif

/**
 * @brief 适配windows和linux的客户端
 */
#ifdef _WIN32
class CWorkerClient : public CWorkerClientImplIOCP
#else
class CWorkerClient : public CWorkerClientImplEpoll
#endif
{
public:

};

#endif
