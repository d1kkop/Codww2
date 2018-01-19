#pragma once


#define NULL_TRACE_ALLOCATIONS (0)

#define NULL_FUNCTION	__FUNCTION__
#define NULL_LINE		__LINE__

#define	NULL_STL		(1)
#define NULL_CLIBS		(1)
#define NULL_WINDOWS	(1)

#define NULL_BUFF_SIZE  4096



#if NULL_WINDOWS 

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <psapi.h>
	#pragma comment(lib, "Psapi.lib")

#endif