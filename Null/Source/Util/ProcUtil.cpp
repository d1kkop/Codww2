#include "ProcUtil.h"
#include "StrUtil.h"


namespace Null
{
	i32 ProcUtil::startProcess(ProcessHandle& handle, const String& absoluteDirectoryPath, const char* exeName, const char* commandLine, bool startSuspended)
	{
		handle = nullptr;

	#if NULL_WINDOWS
		STARTUPINFO startupInfo;
		PROCESS_INFORMATION processInformation;
		DWORD startupFlags = (startSuspended ? CREATE_SUSPENDED : 0) | CREATE_PRESERVE_CODE_AUTHZ_LEVEL;
		memset(&startupInfo, 0, sizeof(STARTUPINFO));
		startupInfo.cb = sizeof(startupInfo);

		String fullName = absoluteDirectoryPath + "/" + exeName;
		if (!CreateProcess(fullName.c_str(),
			(LPSTR)commandLine,
			nullptr,
			nullptr,
			FALSE,
			startupFlags,
			nullptr,
			absoluteDirectoryPath.c_str(),
			&startupInfo,
			&processInformation))
		{
			return (i32)GetLastError();
		}

		handle = processInformation.hProcess;
	#endif

		return 0;
	}

	ProcessHandle ProcUtil::openProcess(const String& name, u32 numProcessesToEvaluate)
	{
		ProcessHandle process = nullptr;
		assert(numProcessesToEvaluate > 0 && name.c_str());

	#if NULL_WINDOWS
		DWORD* processes = new DWORD[numProcessesToEvaluate];
		DWORD bytesWritten;
		if (EnumProcesses(processes, sizeof(DWORD) * numProcessesToEvaluate, &bytesWritten)) 
		{
			for (i32 i = 0; i < i32(bytesWritten / sizeof(DWORD)); i++)
			{
				ProcessHandle tempProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processes[i]);
				if (tempProcess) 
				{
					char processPath[MAX_PATH];
					GetProcessImageFileName(tempProcess, processPath, MAX_PATH);
					if (str_contains(processPath, name))
					{
						process = tempProcess;
						break;
					}
				}
			}
		}
		delete [] processes;
	#else
	#pragma error "not implemented"
	#endif
		
		return process;
	}

	i32 ProcUtil::readProcessMemory(ProcessHandle handle, const char* baseAddress, u64 size, MemoryBlock& block)
	{
		assert(handle);

	#if NULL_WINDOWS
		char* data = (char*) malloc(size);
		if (ReadProcessMemory(handle, baseAddress, data, size, &size))
		{
			block.data  = data;
			block.baseAddress = baseAddress;
			block.size = size;
			block.error = 0;
		}
		else
		{
			DWORD err = GetLastError();
			free(data);
			memset(&block, 0, sizeof(block));
			block.error = err;
			assert(err != 0);
		}
	#endif

		return block.error;
	}

	i32 ProcUtil::writeProcessMemory(ProcessHandle handle, char* address, u64 size, const char* data)
	{
		assert(handle);

	#if NULL_WINDOWS
		if (WriteProcessMemory(handle, (void*)address, data, size, &size))
		{
			return 0;
		}
		else
		{
			DWORD err = GetLastError();
			return err;
		}
	#endif

		return -1;
	}

	i32 ProcUtil::readProcessMemoryRegions(ProcessHandle handle, const char* baseAddress, Array<MemoryBlock>& blocks, u64 maxBytesToRead)
	{
		assert(handle);

	#if NULL_WINDOWS
		MEMORY_BASIC_INFORMATION info;
		u64 bytesRead = 0;
		
		while (true)
		{
			if (!VirtualQueryEx(handle, baseAddress, &info, sizeof(info)))
			{
				return (i32)GetLastError();
			}

			if (maxBytesToRead != InvalidIdx64 && (u64)info.RegionSize + bytesRead > maxBytesToRead)
				return 0;

			bytesRead += info.RegionSize;
			baseAddress += info.RegionSize;

			if (!(info.State == MEM_COMMIT && (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE)))
			{
				continue;
			}

			MemoryBlock block;
			block.data  = (char*)malloc(info.RegionSize);
			block.size  = info.RegionSize;
			block.error	= 0;
			block.baseAddress  = nullptr;

			if (!ReadProcessMemory(handle, info.BaseAddress, block.data, info.RegionSize, &block.size))
			{
				block.error = (i32)GetLastError();
				free(block.data);
				block.data = nullptr;
				block.size = 0;
			}
			else block.baseAddress = (char*) info.BaseAddress;

			blocks.emplace_back(block);
		}

	#else
	#error "not implemented"
	#endif

		return 0;
	}

	i32 ProcUtil::readSingleProcessMemoryBlock(ProcessHandle handle, const char* baseAddress, MemoryBlock& block)
	{
		assert(handle && baseAddress);
		memset(&block, 0, sizeof(block));

	#if NULL_WINDOWS

		MEMORY_BASIC_INFORMATION info;
		if (!VirtualQueryEx(handle, baseAddress, &info, sizeof(info)))
		{
			block.error = (i32)GetLastError();
			return block.error;
		}

		if (!(info.State == MEM_COMMIT && (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE || info.Type == MEM_IMAGE)))
		{
			if (info.State == MEM_COMMIT)
			{
				// try get read access
				DWORD oldProtect;
				if (!VirtualProtectEx(handle, (void*) baseAddress, info.RegionSize, PROCESS_VM_READ, &oldProtect))
				{
					block.error = -1;
					return block.error;
				}
			}
			else
			{
				block.error = -1;
				if (info.State == MEM_FREE) block.error = -2;
				return block.error;
			}
		}

		block.data = (char*) malloc(info.RegionSize);
		if (!ReadProcessMemory(handle, info.BaseAddress, block.data, info.RegionSize, &block.size))
		{
			block.error = (i32)GetLastError();
			free(block.data);
			block.data = nullptr;
			block.size = 0;
		}
		else
		{
			block.baseAddress = (char*)info.BaseAddress;
		}

	#endif

		return block.error;
	}

	i32 ProcUtil::enumerateProcessMemory(ProcessHandle handle, u64 offset,
										 const std::function<bool (char*, u64, bool&)>& cbRegionInfo,
										 const std::function<bool (char*, char*, u64, bool&)>& cbData)

	{
		assert(handle && cbRegionInfo && cbData);

	#if NULL_WINDOWS
		char* p = (char*)offset;
		bool bContinue = true;
		while (bContinue)
		{
			MEMORY_BASIC_INFORMATION info;
			if (!VirtualQueryEx(handle, p, &info, sizeof(info)))
			{
				return (i32)GetLastError();
			}
			p += info.RegionSize;
			if (!(info.State == MEM_COMMIT /*&& (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE)*/))
			{
				continue;
			}
			// see if caller wants to alloc data of this region
			bool bAlloc = false;
			bContinue   = cbRegionInfo((char*)info.BaseAddress, info.RegionSize, bAlloc);
			if (!bContinue) break;
			// alloc block if desired
			if (bAlloc)
			{
				char* blockMem = (char*) malloc(info.RegionSize);
				u64 readSize   = info.RegionSize;
				bool bFree	   = true;
				if (ReadProcessMemory(handle, info.BaseAddress, blockMem, info.RegionSize, &readSize))
				{
					bContinue = cbData((char*)info.BaseAddress, blockMem, readSize, bFree);
				}
				if (bFree) free(blockMem);
			}
		}
	#else
	#error "not implemented"
	#endif

		return 0;
	}

	i32 ProcUtil::setDebugPrivilege(bool enable)
	{
	#if NULL_WINDOWS

		auto fnSetPrivilige = [&](
			HANDLE hToken,          // token handle
			LPCTSTR Privilege,      // Privilege to enable/disable
			BOOL bEnablePrivilege   // TRUE to enable.  FALSE to disable
		)
		{
			TOKEN_PRIVILEGES tp;
			LUID luid;
			TOKEN_PRIVILEGES tpPrevious;
			DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);

			if(!LookupPrivilegeValue( nullptr, Privilege, &luid )) return FALSE;

			// 
			// first pass.  get current privilege setting
			// 
			tp.PrivilegeCount           = 1;
			tp.Privileges[0].Luid       = luid;
			tp.Privileges[0].Attributes = 0;

			AdjustTokenPrivileges(
				hToken,
				FALSE,
				&tp,
				sizeof(TOKEN_PRIVILEGES),
				&tpPrevious,
				&cbPrevious
			);

			if (GetLastError() != ERROR_SUCCESS) return FALSE;

			// 
			// second pass.  set privilege based on previous setting
			// 
			tpPrevious.PrivilegeCount       = 1;
			tpPrevious.Privileges[0].Luid   = luid;

			if(bEnablePrivilege) {
				tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
			}
			else {
				tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED & tpPrevious.Privileges[0].Attributes);
			}

			AdjustTokenPrivileges(
				hToken,
				FALSE,
				&tpPrevious,
				cbPrevious,
				nullptr,
				nullptr
			);

			if (GetLastError() != ERROR_SUCCESS) return FALSE;

			return TRUE;
		};

		auto fnSetPrivilege2 = [&]( 
			HANDLE hToken,  // token handle 
			LPCTSTR Privilege,  // Privilege to enable/disable 
			BOOL bEnablePrivilege  // TRUE to enable. FALSE to disable 
		) 
		{ 
			TOKEN_PRIVILEGES tp = { 0 }; 
			// Initialize everything to zero 
			LUID luid; 
			DWORD cb=sizeof(TOKEN_PRIVILEGES); 
			if(!LookupPrivilegeValue( nullptr, Privilege, &luid ))
				return FALSE; 
			tp.PrivilegeCount = 1; 
			tp.Privileges[0].Luid = luid; 
			if(bEnablePrivilege) { 
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
			} else { 
				tp.Privileges[0].Attributes = 0; 
			} 
			AdjustTokenPrivileges( hToken, FALSE, &tp, cb, nullptr, nullptr ); 
			if (GetLastError() != ERROR_SUCCESS) 
				return FALSE; 

			return TRUE;
		};

		HANDLE hToken;
		DWORD dAcces = TOKEN_ALL_ACCESS; // TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY
		if(!OpenThreadToken(GetCurrentThread(), dAcces, FALSE, &hToken))
		{
			// https://support.microsoft.com/nl-nl/help/131065/how-to-obtain-a-handle-to-any-process-with-sedebugprivilege
			const i32 rtn_error = 13;

			if (GetLastError() == ERROR_NO_TOKEN)
			{
				if (!ImpersonateSelf(SecurityImpersonation))
					return rtn_error;

				if(!OpenThreadToken(GetCurrentThread(), dAcces, FALSE, &hToken)){
					return rtn_error;
				}
			}
			else return rtn_error;
		}

		if (!fnSetPrivilige(hToken, SE_DEBUG_NAME, enable))
		{
			return (i32)GetLastError();
		}

	#endif

		return 0;
	}

}