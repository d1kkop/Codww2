#pragma once

#include "../Common.h"


#define mem_alloc(size) mem_alloc_trace((size), NULL_FUNCTION, NULL_LINE)
#define mem_realloc(mem, size) mem_realloc_trace((mem), (size), NULL_FUNCTION, NULL_LINE)


namespace Null
{
	void*	mem_alloc_trace(u64 size, const char* function, u64 line);
	void*	mem_realloc_trace(void* mem, u64 newSize, const char* function, u64 line);	// If mem is nullptr acts as mem_alloc
	void	mem_free(void* mem);
	void	mem_zero(void* dst, u64 size);
	void	mem_set(void* dst, char value, u64 size);
	void	mem_copy(void* dst, u64 dstSize, const void* src, u64 srcSize);
	bool	mem_equal(const void* left, const void* right, u64 size);
	void*	mem_replace(void* old, u64 newSize, const void* source, u64 copySize);	// If old is nullptr acts as mem_alloc
	void	mem_showAllocationsInOutput();
}
