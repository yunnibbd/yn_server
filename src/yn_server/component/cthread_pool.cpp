#include "cthread_pool.h"
#include "cthread.h"
#include <vector>
#include <list>
#include <iostream>
#include <mutex>
#include <condition_variable>
using namespace std;

//创建的线程池是否全部退出
bool is_stop_all = false;
//存放所有的线程的队列
static vector<CThread*> all_threads;

CThreadPool::CThreadPool()
{

}

//析构
CThreadPool::~CThreadPool()
{
	Close();
}

/**
 * @brief 所有线程OnRun时执行的函数
 * @param pthread 线程对象指针
 * @return
 */
void CThreadPool::ThreadTask(CThread *pthread)
{
	while (pthread->IsRun())
	{
		unique_lock<mutex> tmplock(mux_);
		while (tasks_.empty() && !is_stop_)
			//线程睡眠，等待唤醒
			cv_.wait(tmplock);
		if (is_stop_all)
			break;
		//走到这里必然任务列表中有任务
		TaskType task = tasks_.front();
		tasks_.pop_front();
		//操作完马上解锁
		tmplock.unlock();
		//执行task
		task();
	}
}

/**
 * @brief 初始化线程池，threadnums为线程数量
 * @param threadnums 线程池要创建的线程数量
 * @return bool 是否全部创建成功
 */
bool CThreadPool::Init(int threadnums)
{
	for (int i = 0; i < threadnums; ++i)
	{
		auto th = new CThread();
		threads_.push_back(th);
		th->Start(
			nullptr,
			[this](CThread *pthread) {
				ThreadTask(pthread);
			},
			nullptr
		);
	}
	is_init_ = true;
	thread_num_ = threads_.size();
	return true;
}

/**
 * @brief 关闭线程池
 * @param
 * @return
 */
void CThreadPool::Close()
{
	if (!is_init_) return;
	StopAll();
	for (auto &th : threads_)
	{
		delete th;
	}
	threads_.clear();
	is_init_ = false;
}

/**
 * @brief 停止所有线程
 * @param
 * @return
 */
void CThreadPool::StopAll()
{
	for (auto &th : threads_)
	{
		th->Exit();
	}
	is_stop_ = true;
}

/**
 * @brief 调度任务执行
 * @param task 任务类型
 * @return
 */
void CThreadPool::DispatchTask(TaskType task)
{
	if (!is_init_) return;
	lock_guard<mutex> tmplock(mux_);
	tasks_.push_back(task);
	//激活一个线程来处理这个任务
	cv_.notify_one();
}

/**
 * @brief 创建一个线程池
 * @param
 * @return CThreadPool* 线程池对象指针
 */
CThreadPool *CThreadPoolFactory::Create()
{
	return new CThreadPool();
}

/**
 * @brief 等待所有线程退出
 * @param
 * @return
 */
void CThreadPoolFactory::Wait()
{
	while (!is_stop_all)
	{
		CThread::Sleep(100);
	}
}
