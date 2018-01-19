#pragma once

#include "../Common.h"


namespace Null
{
	using ProcessHandle = void*;

	struct MemoryBlock
	{
		const char* baseAddress;
		char* data;		// should be freed with: 'free', if error != 0, then address = nullptr
		u64 size;
		i32 error;			// if error is not 0, then address is nullptr
	};

	
	struct ProcUtil
	{
		/*	On succes returns 0 and the handle in handleOut is stored. */
		static i32 startProcess(ProcessHandle& handleOut, const String& absoluateDirectoryPath, const char* exeName, const char* commandLine = nullptr, bool startSuspended = false);

		/*	Tries to open a process with ALL access rights. Returns nullptr on failure. */
		static ProcessHandle openProcess(const String& name, u32 numProcessToEvaluate = 300);

		/*	Read from a specific address in process. */
		static i32 readProcessMemory(ProcessHandle handle, const char* address, u64 size, MemoryBlock& block);

		/*	Write at a specific address in process. */
		static i32 writeProcessMemory(ProcessHandle handle, char* address, u64 size, const char* data);

		/*	[BaseAddress] must be valid address in target process memory or nullptr in which
			case it will be rounded to the first region allocated in the process memory.
			Memory in 'blocks' must be freed with 'free' manually.
			Return value is 0 on succes or system error value otherwise. */
		static i32 readProcessMemoryRegions(ProcessHandle handle, const char* baseAddress, Array<MemoryBlock>& blocks, u64 maxBytesToRead = InvalidIdx64);

		/*	Same as above but only reads a single block from offset. */
		static i32 readSingleProcessMemoryBlock(ProcessHandle handle, const char* baseAddress, MemoryBlock& blockOut);

		/*	Iterates through the process's pages and calls cbRegionInfo for each page. 
			If bAlloc is set to true, the data of the process is allocated and read.
			If false is returned from either cbRegionInfo or cbData, the iteration process stops.
			If bFree is set false from cbData, the data is not freed after the callback, otherwise it is. */
		static i32 enumerateProcessMemory(ProcessHandle handle,
										  u64 offset,
										  const std::function<bool (char*, u64, bool& bAlloc)>& cbRegionInfo,
										  const std::function<bool (char*, char*, u64, bool& bFree)>& cbData);


		/*	Enable or disable all priviliges to the calling thread. */
		static i32 setDebugPrivilege(bool enable);
	};

}
