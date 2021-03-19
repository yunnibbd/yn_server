#ifndef __CTHREAD_H__
#define __CTHREAD_H__

#include "csemaphore.h"
#include "./../export_api.h"
#include <thread>
#include <functional>

/**
* @brief 封装的线程类
*/
class COMMONLIB_EXPORT CThread
{	
public:
	typedef std::function<void(CThread *)> Call;

	/**
	 * @brief 开始本线程
	 * @param on_create 线程一开始执行
	 * @param on_run 线程运行执行
	 * @param on_destory 线程最后执行
	 * @return
	 */
	void Start(Call on_create = nullptr, Call on_run = nullptr, Call on_destory = nullptr);

	/**
	 * @brief 停止本线程，会有信号量的参与，更安全
	 * @param
	 * @return
	 */
	void Stop();

	/**
	 * @brief 本线程退出
	 * @param
	 * @return
	 */
	void Exit();

	/**
	 * @brief 本线程是否在运作
	 * @param
	 * @return bool 是否在运行
	 */
	bool IsRun();

	/**
	 * @brief 本线程进入休眠状态
	 * @param
	 * @return
	 */
	void Wait();

	/**
	 * @brief 唤醒本线程
	 * @param
	 * @return
	 */
	void Wakeup();

	/**
	 * @brief 本线程休眠一段时间(单位毫秒)
	 * @param
	 * @return
	 */
	static void Sleep(int msec = 0);
protected:
	/**
	 * @brief 本线程入口函数
	 * @param
	 * @return
	 */
	void Main();

private:
	//线程是否在运行
	bool is_run_ = false;
	//线程的互斥量
	std::mutex mux_;
	//控制线程的休眠
	CSemaphore sem_;
	//线程创建时执行
	Call on_create_ = nullptr;
	//线程运转时执行
	Call on_run_ = nullptr;
	//线程销毁时执行
	Call on_destory_ = nullptr;
};

#endif
