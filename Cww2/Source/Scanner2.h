#pragma once

#include "Null.h"
#include "Vec3.h"

using namespace Null;



struct MemoryEntry3
{
	char* baseAddress;
	u64 offset;
	float value;
	float x, y, z;
};


struct Player
{
	char* baseAddress;
	u64 offset;
	char* data;

	u32 padd0() const		 { return *(u32*)(data + 0 + Off); }
	u32 padd1() const		 { return *(u32*)(data + 16 + Off); }
	u32 padd2() const		 { return *(u32*)(data + 32 + Off); }
	Vec3 getPosition() const { return *(Vec3*)(data + 4 + Off); }
	Vec3 getFacing() const	 { return *(Vec3*)(data + 20 + Off); }
	Vec3 getUpVec() const	 { return *(Vec3*)(data + 72 + Off); }
	float getHeadZ() const	 { return *(float*)(data + 44 + Off); }

	static const u32 Off  = 220;
	static const u32 Size = 316;
};


struct BoneArray
{
	char* baseAddress;
	u64 offset;
	char* data;

	bool operator== (BoneArray& o) const
	{
		if (numBones != o.numBones) return false;
		for (u32 i = 0; i < numBones ; i++)
		{
			Vec3 v1 = *(Vec3*)(data + i*64);
			Vec3 v2 = *(Vec3*)(o.data + i*64);
			if (!(v1 == v2)) return false;
		}
		return true;
	}


	u32 numBones;
	Vec3 pos0()		{ return *(Vec3*)(data + 0); }
	Vec3 pos1()		{ return *(Vec3*)(data + 16); }
};

enum class AroundType
{
	Entry,
	APlayer,
	Search
};

class Scanner2
{
public:
	static constexpr u32 toDiskThreshold = 1000000;

public:
	Scanner2();
	bool init();

	void doNewScan(); // initiates a new scan
	void scanQuats(); // find qouats
	void continueScan();
	void filterHighLow(bool increased, float maxDelta=5);	// filters on increased/decreased
	u64 filterQuats();
	bool keepFilteringQuats();

	u32 getNumEntries();
	void refreshEntries();
	void toMemory();
	void showAround(u32 entryIdx, u32 lowerEntries, u32 higherEntries, AroundType type);
	void findPlayers();
	void refreshPlayers();
	void findBones(u32 offsetDelta);
	void refreshBones();
	void countBonesAround(u32 entryIdx);

	Player makePlayer(char* baseAddress, char* dataCpy, u64 offset);
	BoneArray makeBoneArray(char* baseAddress, char* dataCpy, u64 offset, u32 size, u32 numBones);
	void showPlayers();

private:
	void writeTodisk(u32 fileIdx, bool clearAfter);
	u32 readFromDisk(u32 fileIdx, bool readContent, bool clearBefore);
	void forEachEntry(const std::function<bool (MemoryEntry3&, MemoryBlock&)>& cb);
	void printEntry(MemoryEntry3& e, MemoryBlock& blockSize, u32 idx, u32 diff);
	void printData(u32 idx, i32 diff, char* data, bool asPtr, FILE* f, char* oriPtr, u64 offInBlock, u64 blockSize);

	// filters
	bool nearZero(float f) const;
	bool insideWorld(float f) const;
	bool validFloat(float f) const;
	bool validUp(float f) const;
	bool isIntegral(float f) const;
	bool isValidInt(i32 i) const;

	// player filters
	bool validPosition(const Vec3& pos) const;
	bool validVector(const Vec3& facing) const;

	bool isPlayer(char* data) const;
	bool isBone(char* data) const;
	u32 playerSize() const;


	ProcessHandle m_handle;
	List<MemoryEntry3> m_entries;
	List<Player> m_players;
	List<BoneArray> m_boneArrays;
	Array<char*> m_ptrList;
	u32 m_numFiles;
	bool m_inMemory;
	u64 m_offset;
	
	bool validateBoneArray(BoneArray& boneArray, u32 boneOffset) const;
	bool validateBoneArray2(BoneArray& boneArray) const;
	bool validateBoneArray3(BoneArray& boneArray);
};