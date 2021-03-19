#include "cmemory_pool.h"
#include "clog.h"
#include <stdlib.h>
#include <assert.h>
#include <mutex>
#include <string.h>

CMemoryAlloc::CMemoryAlloc()
	: p_buf_(nullptr), p_header_(nullptr), block_size_(0), block_num_(0)
{

}

CMemoryAlloc::~CMemoryAlloc()
{
	if (p_buf_)
	{
		free(p_buf_);
	}
}

/**
 * @brief 申请一块内存
 * @param nSize 内存大小
 * @return void* 内存指针
 */
void *CMemoryAlloc::AllocMemory(size_t nSize)
{
	std::lock_guard<std::mutex> lg(mutex_);
	if (!p_buf_)
	{
		InitMemory();
	}
	CMemoryBlock* pReturn = nullptr;
	if (nullptr == p_header_)
	{
		pReturn = (CMemoryBlock*)malloc(nSize + sizeof(CMemoryBlock));
		pReturn->bPool = false;
		pReturn->nID = -1;
		pReturn->nRef = 1;
		pReturn->pAlloc = nullptr;
		pReturn->pNext = nullptr;
		//超出内存池时输出
		//LOG_DEBUG("AllocMem: %lx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
	}
	else
	{
		pReturn = p_header_;
		p_header_ = p_header_->pNext;
		assert(0 == pReturn->nRef);
		pReturn->nRef = 1;
	}
	//LOG_DEBUG("AllocMem: %lx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
	//内存前面是这块内存的信息，给用户使用把信息给去掉
	return ((char*)pReturn + sizeof(CMemoryBlock));
}

/**
 * @brief 释放一块内存
 * @param pMem 内存块指针
 * @return
 */
void CMemoryAlloc::FreeMemory(void* pMem)
{
	//内存前面是这块内存的信息，要得到信息就要偏移到内存的前面
	CMemoryBlock* pBlock = (CMemoryBlock*)((char*)pMem - sizeof(CMemoryBlock));
	assert(1 == pBlock->nRef);
	if (pBlock->bPool)
	{//在内存池中的直接放回内存池就行
		std::lock_guard<std::mutex> lg(mutex_);
		if (--pBlock->nRef != 0)
		{
			return;
		}
		pBlock->pNext = p_header_;
		p_header_ = pBlock;
	}
	else
	{//在内存池外直接就是free
		if (--pBlock->nRef != 0)
		{
			return;
		}
		free(pBlock);
	}
}

/**
 * @brief 初始化本内存池
 * @param
 * @return
 */
void CMemoryAlloc::InitMemory()
{
	//LOG_DEBUG("initMemory:block_size_=%d,block_num_=%d\n", block_size_, block_num_);
	assert(nullptr == p_buf_);
	if (p_buf_)
		return;
	size_t offsetSize = block_size_ + sizeof(CMemoryBlock);
	//计算内存池大小(内存单元大小*内存单元数量)
	size_t bufSzie = offsetSize * block_num_;
	//向系统申请池内存
	p_buf_ = (char*)malloc(bufSzie);
	//初始化内存池
	p_header_ = (CMemoryBlock*)p_buf_;
	p_header_->bPool = true;
	p_header_->nID = 0;
	p_header_->nRef = 0;
	p_header_->pAlloc = this;
	p_header_->pNext = nullptr;

	CMemoryBlock* pCur = p_header_;
	CMemoryBlock* pNext = nullptr;
	//从头部开始，让每一块内存单元都指向下一块内存单元
	for (size_t n = 1; n < block_num_; ++n)
	{
		pNext = (CMemoryBlock*)(p_buf_ + n * offsetSize);
		pNext->bPool = true;
		pNext->nID = n;
		pNext->nRef = 0;
		pNext->pAlloc = this;
		pNext->pNext = nullptr;

		pCur->pNext = pNext;
		pCur = pNext;
	}
}

////////////////////////////////////////////////////////////////////

void *CMemoryPool::AllocMem(size_t nSize)
{
	if (nSize <= MAX_MEMORY_SIZE)
	{
		return sz_alloc_[nSize]->AllocMemory(nSize);
	}
	else
	{
		CMemoryBlock* pReturn = (CMemoryBlock*)malloc(nSize + sizeof(CMemoryBlock));
		pReturn->bPool = false;
		pReturn->nID = -1;
		pReturn->nRef = 1;
		pReturn->pAlloc = nullptr;
		pReturn->pNext = nullptr;
		//LOG_DEBUG("AllocMem: %lx, id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		return ((char*)pReturn + sizeof(CMemoryBlock));
	}
}

void CMemoryPool::FreeMem(void* pMem)
{
	//内存前面是这块内存的信息，要得到信息就要偏移到内存的前面
	CMemoryBlock* pBlock = (CMemoryBlock*)((char*)pMem - sizeof(CMemoryBlock));
	//LOG_DEBUG("FreeMem: %lx, id=%d\n", pBlock, pBlock->nID);
	if (pBlock->bPool)
	{//在内存池中的直接放回内存池就行 在FreeMemory会自己转换，所以传递pMem
		pBlock->pAlloc->FreeMemory(pMem);
	}
	else
	{//在内存池外直接就是free
		if (--pBlock->nRef == 0)
			free(pBlock);
	}
}

//增加1内存块的引用计数，暂时用不上
void CMemoryPool::AddRef(void* pMem)
{
	CMemoryBlock* pBlock = (CMemoryBlock*)((char*)pMem - sizeof(CMemoryBlock));
	++pBlock->nRef;
}

/**
 * @brief 初始化内存池映射数组
 * @param nBegin 数组的起始位置
 * @param nEnd 数组的结束位置
 * @param pMem 数组这块区域的项指向Mem
 * @return
 */
void CMemoryPool::Init_szAlloc(int nBegin, int nEnd, CMemoryAlloc* pMem)
{
	for (int i = nBegin; i <= nEnd; ++i)
	{
		sz_alloc_[i] = pMem;
	}
}

CMemoryPool::CMemoryPool()
{
	Init_szAlloc(0, 64, &mem64_);
	Init_szAlloc(65, 128, &mem128_);
	Init_szAlloc(129, 256, &mem256_);
	Init_szAlloc(257, 512, &mem512_);
	Init_szAlloc(513, 1024, &mem1024_);
	Init_szAlloc(1025, 2048, &mem2048_);
	Init_szAlloc(2049, 4096, &mem4096_);
}

CMemoryPool::~CMemoryPool()
{

}
