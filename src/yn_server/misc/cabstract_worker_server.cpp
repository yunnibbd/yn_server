#include "./../component/cthread_pool.h"
#include "./../component/cconfig.h"
#include "./../utils/ctools.h"
#include "./../component/cthread.h"
#include "./../net/cbuffer.h"
#include "./../component/clog.h"
#include "./../3rd/http_parser.h"
#include "./../utils/ccrypt.h"
#include "cabstract_worker_server.h"
#include "cabstract_master_server.h"
#include "cclient.h"
#include <iostream>
using namespace std;

//作为二进制协议包体的最大数据长度限制
#define MAX_PKG_SIZE ((1<<16) - 1)
//作为websocket建立连接开始发送的最大数据长度限制
#define MAX_RECV_SIZE 2047

const char *wb_accept =
"HTTP/1.1 101 Switching Protocols\r\n" \
"Upgrade:websocket\r\n" \
"Connection: Upgrade\r\n" \
"Sec-WebSocket-Accept: %s\r\n" \
"WebSocket-Location: ws://%s:%d/chat\r\n" \
"WebSocket-Protocol:chat\r\n\r\n";

void CAbstractWorkerServer::Init()
{
	//读取配置文件
	CConfig *p_config = CConfig::Get();
	//是否开启心跳检测
	is_check_heart_ = p_config->GetIntDefault("IsCheckHeart", 1);
}

//处理json回调
void CAbstractWorkerServer::OnJson(CClient *pclient, char *data, int data_len)
{
	if (master_server_)
		master_server_->OnJson(pclient, data, data_len);
}

//处理收到的数据
void CAbstractWorkerServer::ReadCB(CClient *pclient, char *data, int data_len)
{
	if (master_server_)
		master_server_->ReadCB(pclient, data, data_len);
}

//开启消息处理服务端线程
void CAbstractWorkerServer::Start()
{
	//如果开启了心跳检测
	if (is_check_heart_ == 1)
	{
		//注册一个定时器 每500毫秒检测所有的客户端心跳超
		RegisterSchedule(
			[this](void *data) {
				CheckHeart();
			}, nullptr, 500
		);
	}

	thread_.Start(
		nullptr,
		[this](CThread *pthread) {
			Main(pthread);
		},
		nullptr
	);
}

void CAbstractWorkerServer::Main(CThread * pthread)
{
	while (pthread->IsRun())
	{
		//判断客户端缓冲区是否有要添加进正式客户端队列中的客户端
		if (clients_buffer_.size() > 0)
		{
			clients_buffer_mutex_.lock();
			for (auto &pclient : clients_buffer_)
			{
				//触发客户端加入事件
				OnJoin(pclient);
				if (pclient)
					//加入正式客户端队列
					clients_.insert(pclient);
			}
			//清理缓冲客户端队列
			clients_buffer_.clear();
			clients_buffer_mutex_.unlock();
		}

		/*if (clients_.empty())
		{
			CThread::Sleep(1000);
			continue;
		}*/

		//处理所有事件
		ProcessEvents();

		//处理消息
		ProcessMsg();
	}
}

//客户端离开事件
void CAbstractWorkerServer::OnLeave(CClient *pclient)
{
	if (master_server_)
		master_server_->OnLeave(pclient);
	//最后再删除
	delete pclient;
}

//服务端接收消息事件
void CAbstractWorkerServer::OnRecv(CClient *pclient)
{
	if (master_server_)
		master_server_->OnRecv(pclient);
}

//心跳检测所有客户端
void CAbstractWorkerServer::CheckHeart()
{
	//LOG_DEBUG("检测所有的定时器 %ld\n", CTools::GetCurMs());
	auto iter = clients_.begin();
	auto end = clients_.end();
	for (;iter != end ;)
	{
		auto pclient = *iter;
		if (pclient->CheckHeart(500))
		{
			cout << "心跳检测超时 client " << pclient->test_id << endl;
			//心跳检测超时
#ifdef _WIN32
			if (pclient->IsPostIoAction())
			{
				//只要关闭这个socket,IOCP就会受到一个未知类型的事件
				pclient->CloseSocket();
				++iter;
				continue;
			}
			else
				//没有向IOCP提交任务,IOCP不会收到通知
				OnLeave(pclient);
#else
			OnLeave(pclient);
#endif
			//从正式客户端队列中移除
			iter = clients_.erase(iter);
			continue;
		}
		++iter;
	}
}

//解析数据到\r\n的位置(用于json)
static bool ParseTailData(char *pkg_data, int recv, int *pkg_size)
{
	if (recv < 2)
		//不可能存放\r\n
		return false;
	int i = 0;
	*pkg_size = 0;
	while (i < recv - 1)
	{
		if (pkg_data[i] == '\r' && pkg_data[i + 1] == '\n')
		{
			*pkg_size = (i + 2);
			return true;
		}
		++i;
	}
	//未找到\r\n
	return false;
}

//使用json方式进行解析
bool CAbstractWorkerServer::OnJsonProtocalRecv(CClient *pclient)
{
	auto p_recv_buf = pclient->p_recv_buf();
	while (p_recv_buf->DataLen() > 0)
	{
		int pack_len = 0;
		auto data = p_recv_buf->Data();
		if (!ParseTailData(data, p_recv_buf->DataLen(), &pack_len))
			//没有解析到\r\n
			break;
		//处理一次json数据
		OnJson(pclient, data, pack_len);
		//移除处理的数据
		p_recv_buf->Pop(pack_len);
	}
	return true;
}

static char header_key[64];
static char client_ws_key[128];
//是否解析到了Sec-WebSocket-Key并存储在client_ws_key中
static bool has_client_key = false;

static int on_header_field(http_parser* p, const char *at, size_t length)
{
	length = (length < 63) ? length : 63;
	strncpy(header_key, at, length);
	header_key[length] = 0;
	//printf("%s:", header_key);
	return 0;
}

static int on_header_value(http_parser* p, const char *at, size_t length)
{
	if (strcmp(header_key, "Sec-WebSocket-Key") != 0)
		return 0;
	length = (length < 127) ? length : 127;

	strncpy(client_ws_key, at, length);
	client_ws_key[length] = 0;
	//printf("%s\n", client_ws_key);
	has_client_key = true;

	return 0;
}

//与websocket客户端进行连接
bool CAbstractWorkerServer::StartWSConnect(CClient *pclient)
{
	auto p_recv_buf = pclient->p_recv_buf();
	http_parser p;
	http_parser_init(&p, HTTP_REQUEST);

	http_parser_settings s;
	http_parser_settings_init(&s);
	s.on_header_field = on_header_field;
	s.on_header_value = on_header_value;

	has_client_key = 0;
	http_parser_execute(&p, &s, p_recv_buf->Data(), p_recv_buf->DataLen());

	if (!has_client_key)
	{
		//还没有接收到websocket的连接请求
		pclient->is_ws_connected = false;
		if (p_recv_buf->DataLen() > MAX_RECV_SIZE)
			//不正常的数据, 关闭本客户端, 后期处理
			return false;
		//没有收到websocket连接请求就返回
		return true;
	}
	//开始建立websocket
	static char key_migic[256];
	const char* migic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	sprintf(key_migic, "%s%s", client_ws_key, migic);

	//存放加密后的数据长度
	int sha1_size = 0;
	int base64_len = 0;
	uint8_t sha1_content[SHA1_DIGEST_SIZE];
	CCrypt::Sha1((uint8_t *)key_migic, (int)strlen(key_migic), sha1_content, &sha1_size);
	char* b64_str = CCrypt::Base64Encode(sha1_content, sha1_size, &base64_len);
	//end 
	strncpy(key_migic, b64_str, base64_len);
	key_migic[base64_len] = 0;
	//将这个http的报文回给websocket连接请求的客户端，
	//生成websocket连接。
	static char accept_buffer[256];
	sprintf(accept_buffer, wb_accept, key_migic, server_ip_, server_port_);
	//下一次循环冲发连接信息发送给客户端
	pclient->SendData(accept_buffer, strlen(accept_buffer));
	//websocket连接标记为已连接, 在服务端已经处理完毕了websocket的握手
	pclient->is_ws_connected = true;
	CCrypt::Base64EncodeFree(b64_str);
	//将所有的前置信息清空
	p_recv_buf->Pop(p_recv_buf->DataLen());
	return true;
}

static bool recv_ws_header(char *pkg_data, int pkg_len, int* pkg_size, int* out_header_size) 
{
	//第一个字节是头，已经判断，跳过;
	//end 

	char *mask = nullptr;
	char *raw_data = nullptr;

	if (pkg_len < 2)
		//websocket 包头没有收完
		return false;

	unsigned int len = pkg_data[1];

	//最高的一个bit始终为1,我们要把最高的这个bit,变为0;
	len = (len & 0x0000007f);
	if (len <= 125) 
	{ 
		//4个mask字节，头才算收完整；
		if (pkg_len < (2 + 4))
			//无法解析出 大小与mask的值
			return false;
		mask = pkg_data + 2; //头字节，长度字节
	}
	else if (len == 126) 
	{
		//后面两个字节表示长度；
		if (pkg_len < (4 + 4))
			//1 + 1 + 2个长度字节 + 4 MASK
			return false;
		len = ((pkg_data[2]) | (pkg_data[3] << 8));
		mask = pkg_data + 2 + 2;
	}
	//1 + 1 + 8(长度信息) + 4MASK
	else if (len == 127) 
	{ 
		//这种情况不用考虑,考虑前4个字节的大小，后面不管;
		if (pkg_len < 14)
			return false;
		unsigned int low = ((pkg_data[2]) | (pkg_data[3] << 8) | (pkg_data[4] << 16) | (pkg_data[5] << 24));
		unsigned int hight = ((pkg_data[6]) | (pkg_data[7] << 8) | (pkg_data[8] << 16) | (pkg_data[9] << 24));
		if (hight != 0) 
			//表示后四个字节有数据int存放不了，太大了，不要
			return false;
		len = low;
		mask = pkg_data + 2 + 8;
	}
	//mask 固定4个字节，所以后面的数据部分
	raw_data = mask + 4;
	*out_header_size = (int)(raw_data - pkg_data);
	*pkg_size = len + (*out_header_size);
	//printf("data length = %d\n", len);
	return true;
}

//解析wbsocket的数据
void CAbstractWorkerServer::ParserWsPack(CClient *pclient, char* body, int body_len, char* mask, int protocal_type)
{
	//使用mask,将数据还原回来；
	for (int i = 0; i < body_len; i++)
		//mask只有4个字节的长度，所以，要循环使用，如果超出，取余就可以
		body[i] = body[i] ^ mask[i % 4];
	//end

	//调用对应的回调，命令分好了
	if (protocal_type == JSON_PROTOCAL)
		OnJson(pclient, body, body_len);
	else if (protocal_type == PROTO_PROTOCAL)
		ReadCB(pclient, body, body_len);
}

//本websocket客户端收到的信息
bool CAbstractWorkerServer::OnWsPackRecv(CClient *pclient)
{
	auto p_recv_buf = pclient->p_recv_buf();
	while (p_recv_buf->DataLen() > 0) {
		int pkg_size = 0;
		int header_size = 0;
		auto data = p_recv_buf->Data();
		auto datalen = p_recv_buf->DataLen();
		if (!recv_ws_header(data, datalen, &pkg_size, &header_size))
			//连请求都接收不完整
			return true;
		if (pkg_size >= MAX_PKG_SIZE)
			//异常的数据包, 关闭此客户端
			return false;
		if (datalen >= pkg_size)
		{
			//表示已经收到至少超过了一个包的数据
			//0x81
			if (data[0] == 0x88)
				//对方要关闭socket
				return false;
			//解析ws数据并根据协议类型调用对应的回调函数
			ParserWsPack(pclient, 
						 data + header_size,
						 pkg_size - header_size, 
						 data + header_size - 4, 
						 pclient->GetProtocalType());
			//移除处理掉的数据
			p_recv_buf->Pop(pkg_size);
			continue;
		}
		//没有收到一个完整的数据包
		break;
	}
	return true;
}

//处理消息
void CAbstractWorkerServer::ProcessMsg()
{
	auto iter = clients_.begin();
	auto end = clients_.end();
	for (; iter != end;)
	{
		auto pclient = *iter;
		if (!pclient->NeedRead()) 
		{
			++iter;
			continue;
		}
		SocketType socket_type = pclient->GetSocketType();
		ProtocalType protocal_type = pclient->GetProtocalType();
		switch (socket_type)
		{
		case TCP_SOCKET_IO:
			if (protocal_type == PROTO_PROTOCAL)
			{
				CBuffer *p_buffer = pclient->p_recv_buf();
				if (p_buffer)
					ReadCB(pclient, p_buffer->Data(), p_buffer->DataLen());
			}
			else if (protocal_type == JSON_PROTOCAL)
			{
				if (!OnJsonProtocalRecv(pclient))
					goto close_client;
			}
			else
				LOG_ERROR("unknow protocal type\n");
			break;
		case WEB_SOCKET_IO:
			//websocket下要先进行建立连接
			if (pclient->is_ws_connected)
			{
				//已经进行了ws连接
				if (!OnWsPackRecv(pclient))
					goto close_client;
			}
			else
			{
				//没有进行ws连接, 先进行ws连接
				if (!StartWSConnect(pclient))
					goto close_client;
			}
			break;
		default:
			LOG_ERROR("unknow socket type\n");
		}
		++iter;
		continue;

close_client:
#ifdef _WIN32
		if (pclient->IsPostIoAction())
		{
			//只要关闭这个socket,IOCP就会受到一个未知类型的事件
			pclient->CloseSocket();
			++iter;
			continue;
		}
		else
			//没有向IOCP提交任务,IOCP不会收到通知
			OnLeave(pclient);
#else
		OnLeave(pclient);
#endif
		//从正式客户端队列中移除
		iter = clients_.erase(iter);
	}
}

//此函数由master服务端调用, 将新接收的客户端加入本消息服务端的客户端缓冲队列
void CAbstractWorkerServer::AddClientToClientsBuffer(CClient *pclient)
{
	clients_buffer_mutex_.lock();
	clients_buffer_.insert(pclient);
	clients_buffer_mutex_.unlock();
}

//获取本消息处理服务端的所有客户端数量
int CAbstractWorkerServer::AllClientNum()
{
	return clients_.size() + clients_buffer_.size();
}

//注册有限定时器
unsigned int CAbstractWorkerServer::RegisterTimer(time_callback cb, void* udata, unsigned int after_msec)
{
	return timer_.RegisterTimer(cb, udata, after_msec);
}

//注册无限次定时器
unsigned int CAbstractWorkerServer::RegisterSchedule(time_callback cb, void* udata, unsigned int after_msec)
{
	return timer_.RegisterSchedule(cb, udata, after_msec);
}

//取消定时器
void CAbstractWorkerServer::UnRegisterTimer(unsigned int timerid)
{
	timer_.UnRegisterTimer(timerid);
}
