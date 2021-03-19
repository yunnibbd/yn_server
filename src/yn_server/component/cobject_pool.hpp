#ifndef __COBJECTPOOL_HPP__
#define __COBJECTPOOL_HPP__
#include <stdlib.h>
#include <mutex>
#include "clog.h"
#include <assert.h>

/*
	模板参数：
		1. 对象池类型
		2. 对象池的大小
*/
template<typename Type, size_t nPoolSize>
class CAbstractObjectPool final
{
public:
	CAbstractObjectPool() : p_header(nullptr), p_buf_(nullptr)
	{
		InitPool();
	}

	~CAbstractObjectPool()
	{
		if (p_buf_)
			delete[] p_buf_;
	}

	/**
	 * @brief 申请对象内存
	 * @param nSize 申请的内存大小
	 * @return void* 内存指针
	 */
	void* AllocObjectMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lg(mutex_);
		NodeHeader* pReturn = nullptr;
		if (nullptr == p_header)
		{
			pReturn = (NodeHeader*)new char[sizeof(Type) * sizeof(NodeHeader)];
			pReturn->bPool = true;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pNext = nullptr;
		}
		else
		{
			pReturn = p_header;
			p_header = p_header->pNext;
			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;
		}
		//LOG_DEBUG("AllocObjectMemory: %lx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		return (char*)pReturn + sizeof(NodeHeader);
	}

	/**
	 * @brief 释放内存块对象
	 * @param pMem 内存块对象指针
	 * @return
	 */
	void FreeObjectMemory(void* pMem)
	{
		//内存前面是这块内存的信息，要得到信息就要偏移到内存的前面
		NodeHeader* pBlock = (NodeHeader*)((char*)pMem - sizeof(NodeHeader));
		//LOG_DEBUG("FreeObjectMemory: %lx, id=%d\n", pBlock, pBlock->nID);
		assert(1 == pBlock->nRef);
		if (pBlock->bPool)
		{//在内存池中的直接放回内存池就行
			std::lock_guard<std::mutex> lg(mutex_);
			if (--pBlock->nRef != 0)
				return;
			pBlock->pNext = p_header;
			p_header = pBlock;
		}
		else
		{//在对象池外直接就是free
			if (--pBlock->nRef != 0)
				return;
			delete[] pBlock;
		}
	}

protected:
	struct NodeHeader
	{
		//下一块位置
		NodeHeader* pNext;
		//内存块编号
		int nID;
		//引用次数
		char nRef;
		//是否在对象池中
		bool bPool;
	private:
		//预留
		char c1;
		char c2;
	};

private:
	/**
	 * @brief 初始化对象池
	 * @param
	 * @return
	 */
	void InitPool()
	{
		assert(nullptr == p_buf_);
		if (p_buf_)
			return;
		//申请池的内存, 大小为 对象池类型大小 乘以 (对象池大小 加上 头部信息)
		size_t offSize = sizeof(Type) + sizeof(NodeHeader);
		p_buf_ = new char[nPoolSize * offSize];
		//初始化对象池
		p_header = (NodeHeader*)p_buf_;
		p_header->nID = 0;
		p_header->bPool = true;
		p_header->nRef = 0;
		p_header->pNext = nullptr;

		NodeHeader* pCur = p_header;
		NodeHeader* pNext = nullptr;
		for (size_t n = 1; n < nPoolSize; ++n)
		{
			pNext = (NodeHeader*)(p_buf_ + offSize * n);
			pNext->nID = n;
			pNext->bPool = true;
			pNext->nRef = 0;
			pNext->pNext = nullptr;

			pCur->pNext = pNext;
			pCur = pNext;
		}
	}

private:
	//头部
	NodeHeader* p_header;
	//对象池内存地址
	char* p_buf_;
	//互斥量
	std::mutex mutex_;
};

/**
 * @brief 以下是对外的接口类
 */
template<typename Type, size_t nPoolSize>
class CObjectPool
{
public:
	void *operator new(size_t size)
	{
		return ObjectPool()->AllocObjectMemory(size);
	}

	void operator delete(void* p)
	{
		ObjectPool()->FreeObjectMemory(p);
	}

	template<typename... Args>
	static Type *CreateObject(Args... args)
	{
		//调用本类重载的new方法
		Type* obj = new Type(args...);
		return obj;
	}

	static void DestoryObject(Type *obj)
	{
		//调用本类重载的delete方法
		delete obj;
	}

private:
	typedef CAbstractObjectPool<Type, nPoolSize> ClassTPool;
	static ClassTPool *ObjectPool()
	{
		//单例对象池CAbstractObjectPool对象
		static ClassTPool cPool;
		return &cPool;
	}
};

#endif
