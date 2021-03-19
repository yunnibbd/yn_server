#include "csemaphore.h"
using namespace std;

/**
 * @brief 开始阻塞本线程
 * @param
 * @return
 */
void CSemaphore::wait()
{
	unique_lock<mutex> lock(mux_);
	//wait_一开始是0
	if (--wait_ < 0)
	{
		condition_.wait(lock, [this]() {
			//一开始wakeup_=0，会休眠，在唤醒之前要++wakeup_
			return wakeup_ > 0;
		});
		--wakeup_;
	}
}

/**
 * @brief 唤醒阻塞的本线程
 * @param
 * @return
 */
void CSemaphore::wakeup()
{
	std::lock_guard<std::mutex> lock(mux_);
	//如果加完后是负数或者0，代表wait过了
	if (++wait_ <= 0)
	{
		++wakeup_;
		condition_.notify_one();
	}
}
