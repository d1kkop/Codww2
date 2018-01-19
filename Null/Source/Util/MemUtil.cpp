#include "MemUtil.h"
#include "Container.h"
#include "StrUtil.h"


namespace Null
{
	static Map<void*, Pair<const char*, u64>> g_allocatedMemory;
	static std::mutex m_allocMutex;


	void* mem_alloc_trace(u64 size, const char* function, u64 line)
	{
		void* ptr = malloc(size);
	#if NULL_TRACE_ALLOCATIONS
		std::lock_guard<std::mutex> lk(m_allocMutex);
		map_insert(g_allocatedMemory, ptr, std::make_pair(function, line));
	#endif
		return ptr;
	}

	void* mem_realloc_trace(void* mem, u64 newSize, const char* function, u64 line)
	{
		void* ptr = realloc(mem, newSize);
	#if NULL_TRACE_ALLOCATIONS
		std::lock_guard<std::mutex> lk(m_allocMutex);
		map_remove(g_allocatedMemory, mem);
		map_insert(g_allocatedMemory, ptr, std::make_pair(function, line));
	#endif
		return ptr;
	}

	void mem_free(void* mem)
	{
	#if NULL_TRACE_ALLOCATIONS
		std::lock_guard<std::mutex> lk(m_allocMutex);
		map_remove(g_allocatedMemory, mem);
	#endif
		free(mem);
	}

	void mem_zero(void* dst, u64 size)
	{
		mem_set(dst, 0, size);
	}
	
	void mem_set(void* dst, char value, u64 size)
	{
		memset(dst, value, size);
	}

	void mem_copy(void* dst, u64 dstSize, const void* src, u64 srcSize)
	{
		assert(dstSize >= srcSize);
		#if NULL_WINDOWS
			memcpy_s(dst, dstSize, src, srcSize);
		#else
			memcpy(dst, src, srcSize);
		#endif
	}

	bool mem_equal(const void* left, const void* right, u64 size)
	{
		return memcmp(left, right, size) == 0;
	}

	void* mem_replace(void* old, u64 newSize, const void* source, u64 copySize)
	{
		old = mem_realloc(old, newSize);
		if (source) mem_copy(old, newSize, source, copySize);
		else mem_zero(old, copySize);
		return old;
	}

	void mem_showAllocationsInOutput()
	{
		for (auto& kvp : g_allocatedMemory)
		{
			const char* function = kvp.second.first;
			u64 line = kvp.second.second;
			String msg = str_format("Memory leak in function %s, line %lld.\n", function, line);
		#if NULL_WINDOWS
			::OutputDebugString(msg.c_str());
		#endif
		}
	}

}
