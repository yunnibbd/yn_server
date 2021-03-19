#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__
#include "./../export_api.h"
#include <vector>
#include <vector>
#include <list>
#include <mutex>
#include <functional>
#include <condition_variable>

using TaskType = std::function<void()>;

class CThread;

/**
 * @brief 一般不直接创建, 使用工厂创建
 */
class COMMONLIB_EXPORT CThreadPool
{
public:
	//构造
	CThreadPool();
	//析构
	~CThreadPool();

	/**
	 * @brief 初始化线程池，threadnums为线程数量
	 * @param threadnums 线程池要创建的线程数量
	 * @return bool 是否全部创建成功
	 */
	bool Init(int threadnums);

	/**
	 * @brief 关闭线程池
	 * @param
	 * @return
	 */
	void Close();

	/**
	 * @brief 停止所有线程
	 * @param
	 * @return
	 */
	void StopAll();

	/**
	 * @brief 调度任务执行
	 * @param task 任务类型
	 * @return
	 */
	void DispatchTask(TaskType task);

	/**
	 * @brief 所有线程OnRun时执行的函数
	 * @param pthread 线程对象指针
	 * @return
	 */
	void ThreadTask(CThread *pthread);

	//线程池是否在运行
	bool IsRun() { return is_init_; }
private:
	//线程池是否已经初始化
	bool is_init_ = false;
	//本线程池线程数量
	int thread_num_ = 0;
	//存放所有的线程对象
	std::vector<CThread *> threads_;
	//存放所有要执行的任务列表
	std::list<TaskType> tasks_;
	//任务列表互斥量
	std::mutex mux_;
	//条件变量
	std::condition_variable cv_;
	//当前线程池是否退出
	bool is_stop_ = false;
};

/**
 * @brief 专门用这个工厂创建线程池
 */
class COMMONLIB_EXPORT CThreadPoolFactory
{
public:
	/**
	 * @brief 创建一个线程池
	 * @param
	 * @return CThreadPool* 线程池对象指针
	 */
	static CThreadPool *Create();

	/**
	 * @brief 等待所有线程退出
	 * @param
	 * @return
	 */
	static void Wait();
};

#endif
