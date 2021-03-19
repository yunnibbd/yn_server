#ifndef __CSEMAPHORE_H__
#define __CSEMAPHORE_H__

#include "./../export_api.h"
#include <thread>
#include <mutex>
#include <condition_variable>

/**
* @brief 封装的信号量类
*/
class COMMONLIB_EXPORT CSemaphore
{
public:
	/**
	 * @brief 开始阻塞本线程
	 * @param
	 * @return
	 */
	void wait();

	/**
	 * @brief 唤醒阻塞的本线程
	 * @param
	 * @return
	 */
	void wakeup();
private:
	std::mutex mux_;
	std::condition_variable condition_;
	//唤醒计数
	int wakeup_ = 0;
	//休眠计数
	int wait_ = 0;
};

#endif
