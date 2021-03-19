#ifndef __COBJECTPOOL_HPP__
#define __COBJECTPOOL_HPP__
#include <stdlib.h>
#include <mutex>
#include "clog.h"
#include <assert.h>

/*
	ģ�������
		1. ���������
		2. ����صĴ�С
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
	 * @brief ��������ڴ�
	 * @param nSize ������ڴ��С
	 * @return void* �ڴ�ָ��
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
	 * @brief �ͷ��ڴ�����
	 * @param pMem �ڴ�����ָ��
	 * @return
	 */
	void FreeObjectMemory(void* pMem)
	{
		//�ڴ�ǰ��������ڴ����Ϣ��Ҫ�õ���Ϣ��Ҫƫ�Ƶ��ڴ��ǰ��
		NodeHeader* pBlock = (NodeHeader*)((char*)pMem - sizeof(NodeHeader));
		//LOG_DEBUG("FreeObjectMemory: %lx, id=%d\n", pBlock, pBlock->nID);
		assert(1 == pBlock->nRef);
		if (pBlock->bPool)
		{//���ڴ���е�ֱ�ӷŻ��ڴ�ؾ���
			std::lock_guard<std::mutex> lg(mutex_);
			if (--pBlock->nRef != 0)
				return;
			pBlock->pNext = p_header;
			p_header = pBlock;
		}
		else
		{//�ڶ������ֱ�Ӿ���free
			if (--pBlock->nRef != 0)
				return;
			delete[] pBlock;
		}
	}

protected:
	struct NodeHeader
	{
		//��һ��λ��
		NodeHeader* pNext;
		//�ڴ����
		int nID;
		//���ô���
		char nRef;
		//�Ƿ��ڶ������
		bool bPool;
	private:
		//Ԥ��
		char c1;
		char c2;
	};

private:
	/**
	 * @brief ��ʼ�������
	 * @param
	 * @return
	 */
	void InitPool()
	{
		assert(nullptr == p_buf_);
		if (p_buf_)
			return;
		//����ص��ڴ�, ��СΪ ��������ʹ�С ���� (����ش�С ���� ͷ����Ϣ)
		size_t offSize = sizeof(Type) + sizeof(NodeHeader);
		p_buf_ = new char[nPoolSize * offSize];
		//��ʼ�������
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
	//ͷ��
	NodeHeader* p_header;
	//������ڴ��ַ
	char* p_buf_;
	//������
	std::mutex mutex_;
};

/**
 * @brief �����Ƕ���Ľӿ���
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
		//���ñ������ص�new����
		Type* obj = new Type(args...);
		return obj;
	}

	static void DestoryObject(Type *obj)
	{
		//���ñ������ص�delete����
		delete obj;
	}

private:
	typedef CAbstractObjectPool<Type, nPoolSize> ClassTPool;
	static ClassTPool *ObjectPool()
	{
		//���������CAbstractObjectPool����
		static ClassTPool cPool;
		return &cPool;
	}
};

#endif
