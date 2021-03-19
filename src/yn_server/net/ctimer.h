#ifndef __TIMER_H__
#define __TIMER_H__

#include "./../export_api.h"
#include <stdio.h>
#include <string.h>
#include <map>
#include <stdlib.h>
#include <functional>

typedef std::function<void(void*)> time_callback;

/**
 * @brief 定时器类
 */
class COMMONLIB_EXPORT CTimer
{
private:
	struct Timer {
		time_callback on_timer;				   //timer回调函数的指针;
		void* udata;                           //回调函数的数据指针;

		unsigned int duration; 		           //触发的时间间隔
		unsigned int end_time_stamp;           //结束的时间戳,毫秒;
		int repeat;                            //重复的次数，-1,表示一直调用;
	};
public:
	//构造
	CTimer();

	//析构
	~CTimer();

	//注册有次数的定时器
	unsigned int RegisterTimer(time_callback cb, void* udata, unsigned int after_msec);

	//注册永久定时器
	unsigned int RegisterSchedule(time_callback cb, void* udata, unsigned int after_msec);

	//注销定时器
	void UnRegisterTimer(unsigned int timeid);

	//处理所有定时器并返回这次将要休眠的时间
	int ProcessTimer();

private:
	//存放键为时间值为回调函数的容器
	//键是毫秒
	std::multimap<unsigned int, Timer*> timer_list_;
	unsigned int cur_running_timer_ = 0;
};

#endif
