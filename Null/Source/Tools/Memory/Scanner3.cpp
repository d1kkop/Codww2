#include "Scanner3.h"
#include "../../Util/Container.h"
#include "../../Util/StrUtil.h"
#include "../../Util/MemUtil.h"
#include "../../Util/FileUtil.h"


namespace Null
{
	// ------ MemEntry ---------------------------------------------------------------------------------------------------------------------------------

	void MemEntry::initValue(const char* _baseAddress, u64 _offset)
	{
		baseAddress = _baseAddress;
		offset = _offset;
		typeId = 0;
		mem_zero(&value, sizeof(CompareValue));
	}

	void MemEntry::updateValue(const char* data, u32 pattern_size, ValueType vt)
	{
		if (!(vt == ValueType::String || vt == ValueType::Memory))
		{
			mem_copy(&value.d, pattern_size, data, pattern_size);
		}
		else
		{
			if (vt == ValueType::Memory) value.ptr = (char*)mem_replace(value.ptr, pattern_size, data, pattern_size);
			else value.ptr = str_replace(value.ptr, pattern_size+1, data);
		}
	}
	
	bool MemEntry::update(ProcessHandle handle, u32 pattern_size, ValueType vt)
	{
		MemoryBlock block;
		bool bSucceeded = false;
		if (ProcUtil::readProcessMemory(handle, baseAddress + offset, pattern_size, block) == 0)
		{
			updateValue(block.data, pattern_size, vt);
			bSucceeded = true;
		}
		mem_free(block.data);
		return bSucceeded;
	}

	const char* MemEntry::getValueAsData(ValueType vt) const
	{
		if (vt == ValueType::String || vt == ValueType::Memory) return (const char*)value.ptr;
		else return (const char*)&value.d;
	}

	void MemEntry::deleteValue(ValueType vt)
	{
		if (vt == ValueType::Memory || vt == ValueType::String) 
		{
			mem_free(value.ptr);
		}
	}


	// ------ Scan ---------------------------------------------------------------------------------------------------------------------------------

	void Scan::addEntry(char* baseAddress, u64 offset)
	{
		MemEntry entry;
		entry.initValue(baseAddress, offset);
		std::lock_guard<std::recursive_mutex> lk(m_lock);
		m_entries.emplace_back(entry);
	}

	const MemEntry* Scan::firstEntry() const
	{ 
		std::lock_guard<std::recursive_mutex> lk(m_lock);
		if (!m_entries.empty()) return &m_entries.front(); 
		return nullptr;
	}

	const MemEntry* Scan::lastEntry() const
	{ 
		std::lock_guard<std::recursive_mutex> lk(m_lock);
		if (!m_entries.empty()) return &m_entries.back(); 
		return nullptr;
	}

	void Scan::forEachEntry(const Function<bool (MemEntry& e)>& cb)
	{
		std::lock_guard<std::recursive_mutex> lk(m_lock);
		for (auto& e : m_entries)
		{
			if (!cb(e)) break;
		}
	}

	void Scan::removeDuplicates(ValueType vt, const Function<bool (MemEntry& a, MemEntry& b)>& cb)
	{
		std::lock_guard<std::recursive_mutex> lk(m_lock);
		for (u64 next = 0; next < m_entries.size(); next++)
		{
			u64 i = 0;
			MemEntry* e = nullptr;
			auto it = m_entries.begin(); 
			for (; it != m_entries.end(); it++, i++)
			{
				if (i == next) 
				{
					e = &(*it);
					it++;
					break;
				}
			}
			for (; it != m_entries.end();)
			{
				if (cb(*e, *it))
				{
					MemEntry& other = (*it);
					other.deleteValue(vt);
					it = m_entries.erase(it);
					continue;
				}
				else it++;
			}
		}
	}

	void Scan::freeEntries()
	{
		std::lock_guard<std::recursive_mutex> lk(m_lock);
		if (valueType == ValueType::String || valueType == ValueType::Memory)
		{
			for (auto& e : m_entries) mem_free(e.value.ptr);
		}
		m_entries.clear();
	}

	void Scan::freeEntry(MemEntry& entry)
	{
		std::lock_guard<std::recursive_mutex> lk(m_lock);
		entry.deleteValue(valueType);
		remove_if(m_entries, [&](auto& e) { return &e == &entry; });
	}

	const Null::MemEntry* Scan::getEntryAt(u32 idx) const
	{
		std::lock_guard<std::recursive_mutex> lk(m_lock);
		if (idx >= m_entries.size()) return nullptr;
		return list_at(m_entries, idx);
	}

	Scan::~Scan() { freeEntries(); }


	// ------ Scanner ---------------------------------------------------------------------------------------------------------------------------------
	Scanner3::Scanner3(const String& processName):
		m_handle(nullptr),
		m_scanId(0)
	{
		m_handle = ProcUtil::openProcess(processName.c_str(), 300);
	}

	Scanner3::~Scanner3()
	{
		for (auto& kvp : m_scans) delete kvp.second;
	}

	u32 Scanner3::newScan(u32 pattern_size, u32 alignment, CompareType compareType, ValueType vt, CompareValue low, CompareValue high, bool allowToDisk)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		if (!m_handle) return -1;
		Scan* s = createScan(alignment, pattern_size, vt, allowToDisk);
		u64 offset = 0;
		ProcUtil::enumerateProcessMemory(m_handle, offset, [&](char* baseAddress, u64 size, bool& alloc)
		{
			alloc = true;
			return true;
		}, [&](char* baseAddress, char* dataCpy, u64 size, bool& bFree)
		{
			for (u64 offs=0; offs+pattern_size <= size; offs += alignment)
			{
				char* data = dataCpy + offs;
				if( isValidEntry(data, pattern_size, compareType, vt, low, high) )
				{
					MemEntry entry;
					entry.initValue(baseAddress, offs);
					entry.updateValue(data, pattern_size, vt);
					s->m_entries.emplace_back(entry);
					if (s->m_entries.size() == toDiskThreshold)
					{
						writeToDisk(s->id, s->numFiles++, true);
					}
				}
			}
			bFree = true;
			return true;
		});
		writeToDisk(s->id, s->numFiles++, true);
		return s->id;
	}

	void Scanner3::filterScan(u32 scanIdx, CompareType compareType, CompareValue low, CompareValue high)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return;
		ValueType vt = s->valueType;
		u32 pattern_size = s->patternSize;
		filterGenericScan( scanIdx, 0, 0, [&](const char* newData, const char* oldData, const char* baseAddress, u64 ofsInBlock, u64 blockSize, u32& typeId)
		{
			if (compareType == CompareType::Less || compareType == CompareType::Greater)
			{
				// set low to previous data
				if (vt == ValueType::Memory || vt == ValueType::String) low.ptr = (char*)oldData;
				else mem_copy(&low.d, pattern_size, oldData, pattern_size);
			}
			if (!isValidEntry(newData, pattern_size, compareType, vt, low, high))
			{
				return false; // do not keep
			}
			typeId = 0;
			return true; // keep
		});
	}

	u32 Scanner3::newGenericScan(u32 pattern_size, u32 alignment, u64 offset, u64 maxScanLength, ValueType vt, bool allowToDisk,
								 const Function<bool (const char*, const char*, u64, u64)>& filterCallback,
								 const Function<bool (const char*, u64)>* preBlockScanCallback)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		if (!m_handle) return -1;
		if (!(vt == ValueType::Memory || vt == ValueType::String) && pattern_size > 8) return -2; // invalid size
		u64 endOffset = offset + maxScanLength;
		u64 offsetRegion = 0; // same as other offset but needs seperate counter for first lamda callback
		Scan* s = createScan(alignment, pattern_size, vt, allowToDisk);
		ProcUtil::enumerateProcessMemory(m_handle, offset, [&](char* baseAddress, u64 size, bool& alloc)
		{
			if ((u64)baseAddress > endOffset) return false; // stop
			if (!preBlockScanCallback)
			{
				alloc = true;
			}
			else
			{
				alloc = (*preBlockScanCallback)(baseAddress, size);
			}
			return true; // continue scanning
		}, [&](char* baseAddress, char* dataCpy, u64 size, bool& bFree)
		{
			for (u64 offs=0; offs+pattern_size <= size; )
			{
				if ((u64)baseAddress+offs > endOffset) return false; // stop
				char* data = dataCpy + offs;
				bool bKeepData = filterCallback(baseAddress, data, offs, size);
				if ( bKeepData)
				{
					MemEntry entry;
					entry.initValue(baseAddress, offs);
					entry.updateValue(data, pattern_size, vt);
					s->m_entries.emplace_back(entry);
					if (s->allowToDisk && s->m_entries.size() == toDiskThreshold)
					{
						writeToDisk(s->id, s->numFiles++, true);
					}
					offs += pattern_size;
				} else offs += alignment;
			}
			bFree = true;
			offset += size;
			return true; // continue scanning
		});
		if (s->allowToDisk) writeToDisk(s->id, s->numFiles++, true);
		return s->id;
	}

	void Scanner3::filterGenericScan(u32 scanIdx, u32 entriesLow, u32 entriesHigh, const Function<bool (const char* newData, const char* oldData, const char* baseAddress, u64 ofsInBlock, u64 blockSize, u32& typeId)>& cb)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return;
		if (!m_handle) return;
		assert(cb);
		MemoryBlock block;
		mem_zero(&block, sizeof(block));
		u32 numFiles = s->numFiles;
		u32 pattern_size = s->patternSize;
		for (u32 fileIdx = 0; fileIdx < numFiles; fileIdx++)
		{
			if (s->allowToDisk) readFromDisk(s->id, fileIdx, true, true, 0); // populates to entries
			for (auto it = s->m_entries.begin(); it != s->m_entries.end();)
			{
				MemEntry& e = *it;
				if (block.baseAddress != e.baseAddress || block.error != 0 || e.offset + pattern_size > block.size)
				{ // free cache
					mem_free(block.data);
					i32 err = ProcUtil::readSingleProcessMemoryBlock(m_handle, e.baseAddress, block);
					if (err != 0 || e.baseAddress != block.baseAddress || block.error != 0 || e.offset + pattern_size > block.size)
					{
						if (err == -2)
						{
							e.deleteValue(s->valueType);
							it = s->m_entries.erase(it); // freed memory
						}
						else it++;
						continue;
					}
				}
				const char* newData = block.data + e.offset;
				const char* oldData = e.getValueAsData(s->valueType);
				bool bKeep = cb(newData, oldData, block.baseAddress, e.offset, block.size, e.typeId);
				if (!bKeep)
				{
					for (i32 i = 1; i <= (i32) entriesLow ; i++) // try lower entries
					{
						i32 offs = -i*4;
						if ( e.offset + offs < (u64) block.baseAddress ) continue;
						bKeep = cb(newData, oldData, block.baseAddress, e.offset + offs, block.size, e.typeId);
						if ( bKeep ) 
						{ 
						//	printf("FILTER REFETCH at offset %d\n", offs);
							e.offset += offs;
							break; 
						}
					}
					if (!bKeep) // try higher entries
					{
						for (i32 i = 1; i <= (i32) entriesHigh ; i++)
						{
							i32 offs = i*4;
							if ( e.offset + offs + pattern_size > (u64) block.baseAddress + block.size ) break;
							bKeep = cb(newData, oldData, block.baseAddress, e.offset + offs, block.size, e.typeId);
							if ( bKeep )
							{ 
							//	printf("FILTER REFETCH at offset %d\n", offs);
								e.offset += offs; 
								break;
							}
						}
					}
					if (!bKeep)
					{
						e.deleteValue(s->valueType);
						it = s->m_entries.erase(it);
					}
				}
				if ( bKeep )
				{
					e.updateValue(block.data + e.offset, pattern_size, s->valueType);
					it++;
				}
			}
			// write updated content (delete or value update) back
			if (s->allowToDisk) writeToDisk(s->id, fileIdx, true);
		}
		mem_free(block.data);
	}

	u64 Scanner3::getNumEntries(u32 scanIdx, u32 typeId, bool fromDisk)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return 0;
		u64 total = 0;
		if (s->allowToDisk)
		{
			if (fromDisk)
			{
				// no need to clear list as we will not populate to it
				for (u32 fileIdx = 0; fileIdx < s->numFiles; fileIdx++)
				{
					u64 num = readFromDisk(scanIdx, fileIdx, false, false, typeId);
					total += num;
				}
			}
			else
			{
				// this is 0, if scan was written to disk and no 'toMemory' was called
				total = s->m_entries.size();
			}
		}
		else
		{
			total = s->m_entries.size();
		}
		return total;
	}

	void Scanner3::showScan(u32 scanIdx)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return;
		u32 entryIdx = 0;
		printf("----- Output for scan idx %d -----\n", s->id);
		filterGenericScan(s->id, 0, 0, [&](const char* newData, const char* oldData, const char* baseAddress, u64 ofsInBlock, u64 blockSize, u32& typeId)
		{
			printEntry(entryIdx++, 0, newData, newData, oldData, baseAddress, ofsInBlock, blockSize);
			return true; // keep data
		});
	}

	bool Scanner3::toMemory(u32 scanIdx, u32 typeId)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return false;
		if (!s->allowToDisk) return true; // if not allowed to write to disk, already in memory
		// first count
		s->freeEntries();
		for (u32 fileIdx = 0; fileIdx < s->numFiles; fileIdx++)
		{
			readFromDisk(s->id, fileIdx, true, false, typeId);
		}
		return true;
	}

	void Scanner3::showAround(u32 scanIdx, u32 entryIdx, u32 lowerEntries, u32 higherEntries, bool fromPtrList, MemoryBlock& cache)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		if (!m_handle) return;
		Scan* s = getScan(scanIdx);
		if (!s) return;
		if ((!fromPtrList && entryIdx >= s->m_entries.size()) || (fromPtrList && entryIdx >= s->m_ptrList.size()))
		{
			printf("Entry index out of range.\n");
			return;
		}
		u64 offset = 0;
		MemoryBlock block = cache;
		bool bValidBlock = false;
		if (!fromPtrList)
		{
			const MemEntry* e = list_at(s->m_entries, entryIdx);
			if (cache.error == 0 && cache.data && e->offset < cache.size && e->baseAddress == cache.baseAddress)
			{
				bValidBlock = true;
			}
			else
			{
				// free cache in this case
				mem_free(cache.data);
				bValidBlock = (ProcUtil::readSingleProcessMemoryBlock(m_handle, e->baseAddress, block) == 0);
				cache = block;
			}
			offset = e->offset;
		}
		else
		{
			char* ptr = s->m_ptrList[entryIdx];	
			if (cache.error == 0 && cache.data && ptr >= cache.baseAddress && ptr < cache.baseAddress+cache.size)
			{
				bValidBlock = true;
			}
			else
			{
				// free cache in this case
				mem_free(cache.data);
				bValidBlock = (ProcUtil::readSingleProcessMemoryBlock(m_handle, ptr, block) == 0);
				cache = block;
			}
			offset = ptr - block.baseAddress;
		}
		if (!bValidBlock) return;
		// compute on 4 byte boundary
		u32 total = lowerEntries + higherEntries;
		u32 idx   = 0;
		for (u32 i=0; i<total; i++)
		{
			char* ptr = block.data + offset;
			char* ptrOff = ptr - (i32)lowerEntries*4 + (i *4);
			// check if in bounds
			if (ptrOff >= block.data && ptrOff < (char*)block.data + block.size-4)
			{
				i32 diff = (i32)( ptrOff - ptr );
				char* asPtr = nullptr;
				if (ptrOff+8 <= block.data + block.size)
					asPtr =	*(char**)(ptrOff);
				if (asPtr) s->m_ptrList.emplace_back(asPtr);
				u32 idx  = (u32)s->m_ptrList.size()-1;
				printEntry(idx, diff, ptr, ptrOff, nullptr, block.baseAddress, offset - lowerEntries*4 + (i *4), block.size);
			}
		}
	//	mem_free(block.data);
	}

	void Scanner3::clearPtrList(u32 scanIdx)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return;
		s->m_ptrList.clear();
		s->m_ptrList.shrink_to_fit();
	}

	void Scanner3::deleteScan(u32 scanIdx)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		map_remove(m_scans, s->id);
		delete s;
	}

	Scan* Scanner3::getScan(u32 scanIdx)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = map_get(m_scans, scanIdx);
		return s;
	}

	bool Scanner3::updateEntry(u32 scanIdx, u32 entryIdx)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return false;
		if (entryIdx >= s->m_entries.size()) return false;
		MemEntry* e = (MemEntry*)list_at(s->m_entries, entryIdx);
		return e->update(m_handle, s->getPatternSize(), s->getValueType());
	}

	u32 Scanner3::updateEntries(u32 scanIdx, u32 numLower, u32 numHigher, 
								const Function<bool (const char* baseAddress, u64 ofsInBlock, const char* newData, const char* oldData, u32& typeId)>& removeFilter,
								const Function<void (const MemEntry& memEntry)>& onDelete)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return false;
		u32 numSucceeded = 0;
		ValueType vt = s->getValueType();
		u32 pattern_size = s->getPatternSize();
		u32 alignment = s->alignment;
		for (auto it = s->m_entries.begin(); it != s->m_entries.end();)
		{
			MemEntry& e = *it;
			char* oldData = nullptr; // copy old data so that a comparison between new and old can be made
			oldData  = (char*) mem_replace(oldData, pattern_size, e.getValueAsData(vt), pattern_size);
			u32 res = tryUpdateEntry(s, e, oldData, 0, removeFilter); // first try original offset
			if (res == 0) // should remove
			{
				// however, try lower and higher entries
				for (i32 i = 1; i <= (i32)numLower ; i++) // try lower offsets
				{
					res = tryUpdateEntry(s, e, oldData, -i*alignment, removeFilter);
					if (res == 1)
					{
				//		printf("REFETCHED at %d offset\n", -i*alignment);
						numSucceeded++; 
						break;
					}
				}
				if (res == 0)
				{
					for (u32 i = 1; i <= numHigher ; i++) // try higher offsets
					{
						res = tryUpdateEntry(s, e, oldData, i*alignment, removeFilter);
						if (res == 1)
						{
				//			printf("REFETCHED at %d offset\n", i*alignment);
							numSucceeded++;
							break; 
						}
					}
				}
			} 
			else if (res == 1) numSucceeded++;
			mem_free(oldData);
			if (res == 0) // only delete if match failed, if read error, do nothing
			{
				onDelete(e);
				e.deleteValue(vt);
				it = s->m_entries.erase(it);
			} else it++;
		}
		return numSucceeded;
	}

	void Scanner3::sortEntries(u32 scanIdx, const Function<bool(const char* left, const char* right)>& compare)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return;
		ValueType vt = s->getValueType();
		Array<MemEntry> tempArray;
		to_array(s->m_entries, tempArray);
		if ( vt == ValueType::Memory || vt == ValueType::String )
		{
			std::sort(tempArray.begin(), tempArray.end(), [&](auto& l, auto& r)
			{
				return compare( l.value.ptr, r.value.ptr );
			});
		}
		else
		{
			std::sort(tempArray.begin(), tempArray.end(), [&](auto& l, auto& r)
			{
				return compare( (const char*)&l.value.d, (const char*)&r.value.d );
			});
		}
		s->m_entries.clear();
		to_array(tempArray, s->m_entries);
	}

	Scan* Scanner3::createScan(u32 alignment, u32 pattern_size, ValueType vt, bool allowToDisk)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = new Scan;
		s->id = m_scanId++;
		s->alignment = alignment;
		s->patternSize = pattern_size;
		s->valueType = vt;
		s->allowToDisk = allowToDisk;
		if ( s->allowToDisk ) s->numFiles = 0;
		else s->numFiles = 1;
		map_insert(m_scans, s->id, s);
		return s;
	}

	void Scanner3::writeToDisk(u32 scanIdx, u32 fileIdx, bool clearAfter)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return;
		assert(s->allowToDisk);
		String path = String("temp/entries_") + to_str(scanIdx) + "_" + to_str(fileIdx);
		File f = file_open(path.c_str(), "wb");
		if (f)
		{
			u64 numEntries = s->m_entries.size();
			if (file_write(f, &numEntries, sizeof(u64)) && !file_error(f))
			{
				for (auto& e : s->m_entries)
				{
					file_write(f, &e, sizeof(MemEntry));
					if (!file_error(f))
					{
						if (s->valueType == ValueType::String)
						{
							u32 len = (u32)str_length(e.value.ptr);
							file_write(f, &len, 4);
							file_write(f, e.value.ptr, len);
						}
						else if (s->valueType == ValueType::Memory)
						{
							file_write(f, e.value.ptr, s->patternSize);
						}
					}
					if (file_error(f))
					{
						printf("File write error.\n");
						break;
					}
				}
			}
			file_close(f);
		}
		else perror("File open failed.\n");
		if (clearAfter) s->freeEntries();
	}

	u64 Scanner3::readFromDisk(u32 scanIdx, u32 fileIdx, bool populateToList, bool clearBefore, u32 typeId)
	{
		std::lock_guard<std::recursive_mutex> lk(m_scansMutex);
		Scan* s = getScan(scanIdx);
		if (!s) return 0;
		assert(s->allowToDisk);
		u64 totalEntries = 0;
		if (clearBefore) s->freeEntries();
		String path = String("temp/entries_") + to_str(scanIdx) + "_" + to_str(fileIdx);
		File f = file_open(path.c_str(), "rb");
		if (f)
		{
			// first get num
			u64 numEntries = 0;
			if (file_read(f, &numEntries, sizeof(u64)) && !file_error(f))
			{
				for (u64 i=0; i<numEntries; i++)
				{
					MemEntry e;
					e.value.ptr = nullptr;
					file_read(f, &e, sizeof(MemEntry));
					bool idMatch   = (typeId==0 || e.typeId == typeId);
					bool addToList = populateToList && idMatch;
					if (!file_error(f))
					{
						if (idMatch) totalEntries++;
						// always read entry entirely otherwise next read will not line up
						if (s->valueType == ValueType::String)
						{
							u32 len=0;
							assert(s->patternSize < 8192*2);
							char buff[8192*2+16];
							file_read(f, &len, 4);
							file_read(f, buff, len);
							if (!file_error(f) && addToList) e.value.ptr = str_new(buff);
						}
						else if (s->valueType == ValueType::Memory)
						{
							assert(s->patternSize < 8192*2);
							// first copy into stack mem, much faster if data is not wanted
							char buff[8192*2];
							file_read(f, buff, s->patternSize);
							if(!file_error(f) && addToList)
							{
								e.value.ptr = (char*) mem_alloc(s->patternSize);
								mem_copy(e.value.ptr, s->patternSize, buff, s->patternSize);
							}
						}
						// see if caller wanted the data
						if (!file_error(f) && addToList) s->m_entries.emplace_back(e);
					}
					if (file_error(f))
					{
						printf("File read error.\n");
						break;
					}
				}
			}
			file_close(f);
		}
		else perror("File open failed.\n");
		return totalEntries;
	}

	bool Scanner3::isValidEntry(const char* data, u32 pattern_size, CompareType compareType, ValueType vt, CompareValue &low,  CompareValue &high)
	{
		float asF = *(float*)data;
		i32  asI  = *(u32*)data;
		u32  asUi = *(i32*)data;
		bool bValidEntry = false;
		switch (compareType)
		{
		case CompareType::Exact:
			if (vt == ValueType::String)
			{
				if (str_equal(data, low.ptr)) bValidEntry = true;
			}
			else if (vt == ValueType::Memory)
			{
				if (mem_equal(data, low.ptr, pattern_size)) bValidEntry = true;
			}
			else if (mem_equal(data, &low.d, pattern_size)) bValidEntry = true;
			break;

		case CompareType::Between:
			switch (vt)
			{
			case ValueType::Float:
				if (asF >= low.f && asF <= high.f) bValidEntry = true;
				break;
			case ValueType::Int:
				if (asI >= low.i && asI <= high.i) bValidEntry = true;
				break;
			case ValueType::Uint:
				if (asUi >= low.ui && asUi <= high.ui) bValidEntry = true;
				break;
			}
			break;

		case CompareType::Less:
			switch (vt)
			{
			case ValueType::Float:
				if (asF < low.f) bValidEntry = true;
				break;
			case ValueType::Int:
				if (asI < low.i) bValidEntry = true;
				break;
			case ValueType::Uint:
				if (asUi < low.ui) bValidEntry = true;
				break;
			}
			break;

		case CompareType::Greater: // note, low is correct, high is only used for 'between' match
			switch (vt)
			{
			case ValueType::Float:
				if (asF > low.f) bValidEntry = true;
				break;
			case ValueType::Int:
				if (asI > low.i) bValidEntry = true;
				break;
			case ValueType::Uint:
				if (asUi > low.ui) bValidEntry = true;
				break;
			}
			break;
		}
		return bValidEntry;
	}

	u32 Scanner3::tryUpdateEntry(Scan* scan, MemEntry& e, const char* oldData, i32 dynOffset,
								 const Function<bool (const char* baseAddress, u64 ofsInBlock, const char* newData, const char* oldData, u32& typeId)>& removeFilter)
	{
		u64 origOffset = e.offset;
		e.offset += dynOffset;
		auto vt = scan->valueType;
		bool bUpdateResult = e.update(m_handle, scan->patternSize, vt);
		if (bUpdateResult)
		{
			if (!removeFilter(e.baseAddress, e.offset, e.getValueAsData(vt), oldData, e.typeId))
			{
				e.offset = origOffset;
				return 0; // match fail
			}
			return 1; // match succes
		}
		e.offset = origOffset;
		return -1; // error
	}


	void Scanner3::printEntry(u32 idx, u32 diff, const char* origAddr, const char* newData, const char* oldData, const char* baseAddress, u64 offsInBlock, u64 blockSize)
	{
		float f     = *(float*)newData;
		i32 i	    = *(i32*)newData;
		char* asPtr = nullptr;

		/*if (e.offset % 8 == 0)*/ asPtr = *(char**)newData;
		char str[] = { *newData, *(newData+1), *(newData+2), *(newData+3), '\0' };

		if (i == 0) i = 1111111111;
		if (f == 0 || f < -1e20 || f > 1e20) f = 1.11111111f;

		printf("%p\t %d\t %d\t %d\t %f\t %p\t %s\t %p\t %lld\t %lld\n", origAddr, diff, idx, i, f, asPtr, str, baseAddress, offsInBlock, blockSize);
	}

}