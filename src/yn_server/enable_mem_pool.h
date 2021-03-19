#ifndef __ENABLE_MEM_POOL_H__
#define __ENABLE_MEM_POOL_H__
#include "./component/cmemory_pool.h"

//开头引入此文件代表所有的new和delete行为都由内存池监控

inline void* operator new(size_t size)
{
	return CMemoryPool::Get()->AllocMem(size);
}

inline void operator delete(void* p)
{
	CMemoryPool::Get()->FreeMem(p);
}

inline void* operator new[](size_t size)
{
	return CMemoryPool::Get()->AllocMem(size);
}

inline void operator delete[](void* p)
{
	CMemoryPool::Get()->FreeMem(p);
}

#endif
