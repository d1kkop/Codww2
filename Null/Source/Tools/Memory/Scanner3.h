#pragma once

#include "../../Common.h"
#include "../../Util/ProcUtil.h"
#include "../../Util/Container.h"
#include "../../Util/MemUtil.h"


namespace Null
{
	enum class CompareType
	{
		Exact,
		Less,
		Greater,
		Between
	};

	inline CompareType ct_to_str(const Null::String& s)
	{
		if (s == "exact") return CompareType::Exact;
		else if (s == "less" ) return CompareType::Less;
		else if (s == "greater") return CompareType::Greater;
		else if (s == "between") return CompareType::Between;
		return CompareType::Exact;
	};

	enum class ValueType
	{
		String,
		Float,
		Int,
		Uint,
		Ptr,
		Memory
	};

	inline ValueType vt_to_str(const Null::String& s)
	{
		if (s == "string") return ValueType::String;
		else if (s == "float" ) return ValueType::Float;
		else if (s == "int")  return ValueType::Int;
		else if (s == "uint") return ValueType::Uint;
		else if (s == "ptr")  return ValueType::Ptr;
		else if (s == "memory")  return ValueType::Memory;
		return ValueType::String;
	};

	// Value type and size are deliberately not held here to constrain memory consumption
	struct CompareValue
	{
		float asFloat() const { return f; }
		double asDouble() const { return d; }
		i32 asI32() const { return i; }
		u32 asU32() const { return ui; }
		const char* asPtr() const { return ptr; }

		union
		{
			float f;
			u32 ui;
			i32 i;
			double d;
			char* ptr;	// If valueType is string or memory, ptr should be freed with 'free'
		};
	};

	struct MemEntry
	{
		const char* baseAddress;
		u64 offset;
		CompareValue value;
		u32 typeId;

		bool operator ==(const MemEntry& r)
		{
			return baseAddress == r.baseAddress && offset == r.offset;
		}

	private:
		void initValue(const char* baseAddress, u64 offset);
		void updateValue(const char* data, u32 pattern_size, ValueType vt);
		bool update(ProcessHandle handle, u32 pattern_size, ValueType vt);
		const char* getValueAsData(ValueType vt) const;
		void deleteValue(ValueType vt);

		friend class Scanner3;
		friend class Scan;
	};


	class Scan
	{
	public:
		u32 getId() const { return id; }
		ValueType getValueType() const { return valueType; }
		u32 getPatternSize() const { return patternSize; }
		u32 numEntries() const { std::lock_guard<std::recursive_mutex> lk(m_lock); return (u32)m_entries.size(); }	// Note this returns entries in memory. If entries are stored on disk use Scanner.getNumEntries( scan->getId() )
		void addEntry(char* baseAddress, u64 offset);
		const MemEntry* firstEntry() const;
		const MemEntry* lastEntry() const;
		void forEachEntry(const Function<bool (MemEntry& e)>& cb);
		void removeDuplicates(ValueType vt, const Function<bool (MemEntry& a, MemEntry& b)>& cb);
		void freeEntries();
		void freeEntry(MemEntry& entry);
		const MemEntry* getEntryAt(u32 idx) const;
		~Scan();

	private:
		u32 id;
		bool allowToDisk;
		u32 alignment;
		u32 numFiles;
		u32 patternSize;
		ValueType valueType;
		List<MemEntry> m_entries;
		Array<char*> m_ptrList;
		mutable std::recursive_mutex m_lock;

		friend class Scanner3;
	};


	class Scanner3
	{
	public:
		static constexpr u32 toDiskThreshold = 100000;

	public:
		Scanner3(const String& processName);
		~Scanner3();

		/*	Sets up a new scan using compare type and compare values.
			For a more specific filter, use the 'FilteredScan' method. 
			Returns an id to identify the scan with or -1 on error. */
		u32 newScan(u32 pattern_size, u32 alignment, CompareType compareType, ValueType valueType, CompareValue low, CompareValue high, bool allowToDisk=true);
		void filterScan(u32 scanIdx, CompareType compareType, CompareValue low, CompareValue high);

		/*  For a more generic search with a custom filter (lamda callback) use the below two functions.
			The lamda should return 'true' to keep the entry, false otherwise.
			Returns an id to identify the scan with or -1 on error. */
		u32 newGenericScan(u32 pattern_size, u32 alignment, u64 offset, u64 maxScanLength, ValueType valueType, bool allowToDisk,
						   const Function<bool (const char*, const char*, u64, u64)>& filterCallback,
						   const Function<bool (const char*, u64)>* preBlockScanCallback = nullptr);
		void filterGenericScan(u32 scanIdx, u32 entriesLow, u32 entriesHigh, const Function<bool (const char* newData, const char* oldData, const char* baseAddress, u64 ofsInBlock, u64 blockSize, u32& typeId)>& cb);

		/*	Returns num of entries in saved on disk or currently in memory. 
			If getFromDisk is true, while the scan 'cannot' write to disk it 
			returns the amount currently held in memory. 
			If not getFromDisk is true, the amount in memory is inspected.
			If the scan was written to disk, but toMemory was not called
			for scan, then the result is 0. */
		u64 getNumEntries(u32 scanIdx, u32 typeId, bool getFromDisk=true);

		/*	Formatted prints the results of the scan to stdout.
			Only works if data is in memory. */
		void showScan(u32 scanIdx);

		/*	Puts data in memory if <= 1000 entries remaining of typeId. */
		bool toMemory(u32 scanIdx, u32 typeId);

		/*	Formatted print entries around entry.
			If fromPtrList is false, entryIndex in scan.m_entries is used.
			Otherwise, an address is obtained from scan.m_ptrList. The ptr_list is populated each time a new 'showAround'
			call is made. */
		void showAround(u32 scanIdx, u32 entryIdx, u32 lowerEntries, u32 higherEntries, bool fromPtrList, MemoryBlock& cache);

		/*	Clear the ptr_list from consequtive calls to showAround. */
		void clearPtrList(u32 scanIdx);

		/*	Deleted a former scan result from memory. */
		void deleteScan(u32 scanIdx);
		Scan* getScan(u32 scanIdx);

		/*	Updates a specific entry (indexed based) in a scan. 
			Returns true on succes. */
		bool updateEntry(u32 scanIdx, u32 entryIdx);

		/*	Updates all entries in a scan. 
			A removeFilter can be specified to remove entries while updating them based on new and old data.
			The removeFilter should return true to keep the entry and false otherwise.
			Only returns true if all entries succeeded to update. */
		u32 updateEntries(u32 scanIdx, u32 numLower, u32 numHigher, 
						  const Function<bool (const char* baseAddress, u64 ofsInBlock, const char* newData, const char* oldData, u32& typeId)>& removeFilter,
						  const Function<void (const MemEntry& memEntry)>& onDelete);

		/*	Can only be done when put in memory. */
		void sortEntries(u32 scanIdx, const Function<bool (const char* left, const char* right)>& compare);

		ProcessHandle getHandle() const { return m_handle; }

		Scan* createScan(u32 alignment, u32 pattern_size, ValueType vt, bool allowToDisk);

	private:
		void writeToDisk(u32 scanIdx, u32 fileIdx, bool clearAfter);
		u64  readFromDisk(u32 scanIdx, u32 fileIdx, bool populateToList, bool clearBefore, u32 typeId);

		bool isValidEntry(const char* data, u32 pattern_size, CompareType compareType, ValueType vt, CompareValue &low,  CompareValue &high);
		u32  tryUpdateEntry(Scan* scan, MemEntry& e, const char* oldData, i32 dynOffset,
							const Function<bool (const char* baseAddress, u64 ofsInBlock, const char* newData, const char* oldData, u32& typeId)>& removeFilter);
		void printEntry(u32 idx, u32 ankerOffset, const char* oriPtr, const char* newData, const char* oldData, const char* baseAddress, u64 ofsInBlock, u64 blockSize);

		ProcessHandle m_handle;
		Map<u32, Scan*> m_scans;
		u32 m_scanId;
		mutable std::recursive_mutex m_scansMutex;
	};
}