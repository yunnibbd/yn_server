#include "ctimer.h"
#include "./../utils/ctools.h"
#include <limits.h>
using namespace std;

//构造
CTimer::CTimer()
{
	
}

//析构
CTimer::~CTimer()
{
	auto begin = timer_list_.begin();
	auto end = timer_list_.end();
	for (; begin != end; ++begin)
	{
		delete begin->second;
	}
	timer_list_.clear();
}

//注册有次数的定时器
unsigned int CTimer::RegisterTimer(time_callback cb, void* udata, unsigned int after_msec) {
	Timer* timer = new Timer();
	timer->on_timer = cb;
	timer->duration = after_msec;
	timer->end_time_stamp = CTools::GetCurMs() + timer->duration;
	timer->udata = udata;
	timer->repeat = 1;
	auto pair = std::make_pair(timer->end_time_stamp, timer);
	timer_list_.insert(pair);
	return timer->end_time_stamp;
}

//注册永久定时器
unsigned int CTimer::RegisterSchedule(time_callback cb, void* udata, unsigned int after_msec) {
	Timer* timer = new Timer();
	timer->on_timer = cb;
	timer->duration = after_msec;
	timer->end_time_stamp = CTools::GetCurMs() + timer->duration;
	timer->udata = udata;
	timer->repeat = -1;
	auto pair = std::make_pair(timer->end_time_stamp, timer);
	timer_list_.insert(pair);
	return timer->end_time_stamp;
}

//注销定时器
void CTimer::UnRegisterTimer(unsigned int timeid) {
	if (cur_running_timer_ == timeid)
	{//阻止触发自己的时候删除自己
		return;
	}
	auto iter = timer_list_.find(timeid);
	if (iter != timer_list_.end())
	{ //找到了这个定时器
		delete iter->second;
		timer_list_.erase(iter);
	}
}

//处理所有定时器并返回这次将要休眠的时间
int CTimer::ProcessTimer() {
	unsigned int cur_time = CTools::GetCurMs();
	auto begin = timer_list_.begin();
	auto end = timer_list_.end();
	while (begin != end)
	{
		cur_running_timer_ = begin->first;
		auto cur_timer = begin->second;
		if (cur_running_timer_ <= cur_time)
		{
			//到达触发时间
			cur_timer->on_timer(cur_timer->udata);
			begin = timer_list_.erase(begin);
			if (cur_timer->repeat > 0)
			{
				//表示是有限的触发次数
				--cur_timer->repeat;
				if (cur_timer->repeat == 0)
				{
					//该定时器已经失效
					delete cur_timer;
					continue;
				}
			}

			//在以下表示是无限次的定时器或者是还有触发次数的定时器
			//更新end_time_stamp并重新添加
			cur_timer->end_time_stamp = cur_timer->duration + CTools::GetCurMs();
			timer_list_.insert(make_pair(cur_timer->end_time_stamp, cur_timer));
		}
		//map默认小端堆排序
		break;
	}
	cur_running_timer_ = 0;
	if (timer_list_.size() > 0)
		return timer_list_.begin()->first - CTools::GetCurMs();
	else
		return -1;
}
