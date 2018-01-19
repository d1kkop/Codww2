#include "Scanner2.h"


Scanner2::Scanner2():
	m_handle(nullptr),
	m_numFiles(0),
	m_inMemory(false),
	m_offset(0)
{

}

bool Scanner2::init()
{
//	m_entries.reserve( 1024 * 1024 * 15 );

	m_handle = ProcUtil::openProcess("s2_mp64_ship.exe", 300);
	if (!m_handle) return false;

	return true;
}

void Scanner2::doNewScan()
{
	if (!m_handle) return;
	m_entries.clear(); // clear prev result
	m_numFiles = 0;
	m_inMemory = false;
	m_offset = 0;
	ProcUtil::enumerateProcessMemory(m_handle, m_offset, [&](char* baseAddress, u64 size, bool& alloc)
	{
		alloc = true;
		return true;
	}, [&](char* baseAddress, char* dataCpy, u64 size, bool& bFree)
	{
		u64 offs = 0;
		for (; offs < size-3; offs += 4) // 4 byte alignment
		{
			float f = *(float*)(dataCpy + offs);
			//if (validFloat(f))
			if (validUp(f))
			{
				MemoryEntry3 entry;
				entry.baseAddress = baseAddress;
				entry.offset = offs;
				entry.value  = f;
				m_entries.emplace_back(entry);
				if (m_entries.size() == toDiskThreshold)
				{
					writeTodisk(m_numFiles++, true);
				}
			}
		}
		bFree = true;
		return true;
	});
	writeTodisk(m_numFiles++, true);
}

void Scanner2::scanQuats()
{
	if (!m_handle) return;
	m_entries.clear(); // clear prev result
	m_numFiles = 0;
	m_inMemory = false;
	m_offset = 0;
	u64 numQUats = 0;
	ProcUtil::enumerateProcessMemory(m_handle, m_offset, [&](char* baseAddress, u64 size, bool& alloc)
	{
		alloc = true;
		return true;
	}, [&](char* baseAddress, char* dataCpy, u64 size, bool& bFree)
	{
		u64 offs = 0;
		for (; offs < size-15; offs += 4) // 4 byte alignment
		{
			float x = *(float*)(dataCpy + offs);
			float y = *(float*)(dataCpy + offs+4);
			float z = *(float*)(dataCpy + offs+8);
			float w = *(float*)(dataCpy + offs+12);
			//if (validFloat(f))
			if ( fabs(sqrt(x*x + y*y + z*z + w*w)-1.f) < 0.05f )
			{
				MemoryEntry3 entry;
				entry.baseAddress = baseAddress;
				entry.offset = offs;
				entry.value  = w;
				entry.x = x;
				entry.y = y;
				entry.z = z;
				m_entries.emplace_back(entry);
				numQUats++;
				if (m_entries.size() == toDiskThreshold)
				{
					writeTodisk(m_numFiles++, true);
				}
			}
		}
		bFree = true;
		return true;
	});
	writeTodisk(m_numFiles++, true);
	printf("Num quats %lld\n", numQUats);
}


void Scanner2::continueScan()
{
	if (!m_handle) return;
	// continue at offset
	ProcUtil::enumerateProcessMemory(m_handle, m_offset, [&](char* baseAddress, u64 size, bool& alloc)
	{
		alloc = true;
		return true;
	}, [&](char* baseAddress, char* dataCpy, u64 size, bool& bFree)
	{
		u64 offs = 0;
		for (; offs < size-3; offs += 4) // 4 byte alignment
		{
			float f = *(float*)(dataCpy + offs);
			//if (validFloat(f))
			if (validUp(f))
			{
				MemoryEntry3 entry;
				entry.baseAddress = baseAddress;
				entry.offset = offs;
				entry.value  = f;
				m_entries.emplace_back(entry);
				if (m_entries.size() == toDiskThreshold)
				{
					writeTodisk(m_numFiles++, true);
				}
			}
		}
		bFree = true;
		return true;
	});
	writeTodisk(m_numFiles++, true);
}

void Scanner2::filterHighLow(bool mustBeHigher, float delta)
{
	forEachEntry( [&](MemoryEntry3& e, MemoryBlock& block)
	{
		float f = *(float*)(block.data + e.offset);
		float dt  = f-e.value;
		float fdt = fabs(dt);

		if (
			!validUp(f) || 
			(fdt < 0.1f) || 
			(mustBeHigher && (dt < 0 || dt > delta)) || 
			(!mustBeHigher && (dt > 0 || dt < -delta))
			)
		{
			return true; // erase
		}
		else 
		{
			// update
			e.value = f;
			return false; // no remove
		}
	});
}

u64 Scanner2::filterQuats()
{
	u64 numEntries = 0;
	forEachEntry( [&](MemoryEntry3& e, MemoryBlock& block)
	{
		float x = *(float*)(block.data + e.offset);
		float y = *(float*)(block.data + e.offset+4);
		float z = *(float*)(block.data + e.offset+8);
		float w = *(float*)(block.data + e.offset+12);

		float l = sqrt(x*x + y*y + z*z + w*w);

		if (fabs(l-1) > 0.05f ||
			fabs(x) < 0.001f || fabs(y) < 0.001f || fabs(z) < 0.001f ||
			( x == e.x && y==e.y && z==e.z && w == e.value ) )
		{
			return true; // erase
		}
		else 
		{
			// update
			e.x = x;
			e.y = y;
			e.z = z;
			e.value = w;
			numEntries++;
			return false; // no remove
		}
	});
	return numEntries;
}

bool Scanner2::keepFilteringQuats()
{
	 u64 entries = filterQuats();
	 while (entries > 290)
	 {
		 entries = filterQuats();
		 printf("num quats: %lld\n", entries);
	 }
	 return true;
}

u32 Scanner2::getNumEntries()
{
	if (!m_inMemory)
	{
		u32 total = 0;
		for (u32 fileIdx = 0; fileIdx < m_numFiles; fileIdx++)
		{
			size_t num = readFromDisk(fileIdx, false, false);
			total += (u32)num;
		}
		return total;
	}
	else
	{
		return (u32)m_entries.size();
	}
}

void Scanner2::refreshEntries()
{
	if (!m_inMemory)
	{
		printf("First put in memory.\n");
		return;
	}
	u32 entryIdx = 0;
	char* prevPtr = nullptr;
	forEachEntry( [&](MemoryEntry3& e, MemoryBlock& block)
	{
		char* ptr = block.data + e.offset;
		u32 diff = (u32) (ptr - prevPtr);
		prevPtr = ptr;
		e.value   = *(float*)ptr;// update
		printEntry(e, block, entryIdx++, diff);
		return false; // no erase
	});
}

void Scanner2::toMemory()
{
	if (m_inMemory) 
	{
		printf("Already in memory.\n");
		return;
	}
	// first count
	u32 numEntries = getNumEntries();
	if (numEntries > 1000)
	{
		printf("Num entries must be 1000 or less.\n");
		return;
	}
	for (u32 fileIdx = 0; fileIdx < m_numFiles; fileIdx++)
	{
		readFromDisk(fileIdx, true, false);
	}
	m_numFiles = 0;
	m_inMemory = true;
}

void Scanner2::showAround(u32 entryIdx, u32 lowerEntries, u32 higherEntries, AroundType type)
{
	if (!m_handle) return;
	if (!m_inMemory && type == AroundType::Entry)
	{
		printf("Entries not in memory.\n");
		return;
	}
	if ((type==AroundType::Entry && entryIdx >= m_entries.size()) || 
		(type==AroundType::Search && entryIdx >= m_ptrList.size()) ||
		(type==AroundType::APlayer && entryIdx >= m_players.size()))
	{
		printf("Entry index out of range.\n");
		return;
	}
	MemoryBlock block;
	bool bValidBlock = false;
	u64 offset = 0;
	if (type==AroundType::Entry)
	{
		const MemoryEntry3* e = list_at(m_entries, entryIdx);
		bValidBlock = (ProcUtil::readSingleProcessMemoryBlock(m_handle, e->baseAddress, block) == 0) && block.data != nullptr;
		offset = e->offset;
	}
	else if(type==AroundType::Search)
	{
		char* ptr = m_ptrList[entryIdx];	
		bValidBlock = (ProcUtil::readSingleProcessMemoryBlock(m_handle, ptr, block) == 0 && block.data != nullptr);
		offset = ptr - block.baseAddress;
	}
	else if(type==AroundType::APlayer)
	{
		const Player* player = list_at(m_players, entryIdx);
		bValidBlock = (ProcUtil::readSingleProcessMemoryBlock(m_handle, player->baseAddress, block) == 0 && block.data != nullptr);
		offset = player->offset;
	}
	if (!bValidBlock) return;
	// compute on 4 byte boundary
	FILE* f;
	fopen_s(&f, "around.txt", "w");
	if (f)
	{
		u32 total = lowerEntries + higherEntries;
		for (u32 i=0; i<total; i++)
		{
			char* ptr = block.data + offset;
			char* ptrOff = ptr - (i32)lowerEntries*4 + (i *4);
			// check if in bounds
			if (ptrOff >= block.data && ptrOff < (char*)block.data + block.size-4)
			{
				char* asPtr = nullptr;
				if (ptrOff < (char*)block.data + block.size-7 && (i%2)==0)
					asPtr =	(char*)*(void**)ptrOff;
				m_ptrList.emplace_back(asPtr);
				u32 idx  = (u32)m_ptrList.size()-1;
				i32 diff = (i32)( ptrOff - ptr );
				printData(idx, diff, ptrOff, asPtr != nullptr, f, nullptr, offset+diff, block.size);
			}
		}
		fflush(f);
		fclose(f);
	}
	free(block.data);
}

void Scanner2::findPlayers()
{
	m_players.clear();
	u64 numPlayers = 0;
	u64 offset = 0;
	ProcUtil::enumerateProcessMemory(m_handle, offset, [&](char* baseAddress, u64 size, bool& bAlloc)
	{
		bAlloc = true;
		return true;
	},  [&](char* baseAddress, char* dataCpy, u64 size, bool& bFree)
	{
		u64 offs = 0;
		for (u64 offs = 0; offs < size-(playerSize()-1); offs += 4)
		{
			if (isPlayer(dataCpy + offs))
			{
			//	if (m_players.size()<100) 
				{
					Player player = makePlayer(baseAddress, dataCpy, offs);
					m_players.emplace_back(player);
				}
				numPlayers++;
			}
		}
		bFree = true;
		return true;
	});
	printf("Num players found %lld\n", numPlayers);
}

void Scanner2::refreshPlayers()
{
	Vec3 pPrev;
	pPrev.x = pPrev.y = pPrev.z = 0;
	for (auto it = m_players.begin(); it != m_players.end(); )
	{
		Player& player = *it;
		MemoryBlock block;
		i32 res = ProcUtil::readProcessMemory(m_handle, player.baseAddress + player.offset, Player::Size, block);
		//i32 res = ProcUtil::readSingleProccesMemoryBlock(m_handle, player.baseAddress, block);
		if (res == 0)// && player.offset < block.size-(playerSize()-1))
		{
			Player& player = *it;
			free(player.data); // free old data
			player.data = block.data;
			if (!isPlayer(block.data) ||
				 player.getPosition().dist(pPrev) < 300)
			{
				it = m_players.erase(it);
				free(block.data);
				continue;
			} 
			pPrev = player.getPosition();
		} 
		// data already if res == 0
		it++;
	}
	printf("Num players after refresh %lld\n", m_players.size());
}

void Scanner2::findBones(u32 offsetDelta)
{
	const u32 offsets [] = {
		///*48,*/ 0, 48, 1374752, 
		0,
		16, 48, 
		16, 48,
		16, 48,
		16, 48,
		// 4
		16, 48,
		16, 48,
		16, 48,
		16, 48,
		// 8
		16, 48,
		16, 48,
		16, 
		// 11
	//	5776, 32, 32, 32, 32, 32, 200096, 48, 48, 320, 48, 48, 320, 48, 48, 320, 48, 48, 320, 48, 48, 320, 48, 48,	13071536
	};
	u32 numOffests = sizeof(offsets) / sizeof(u32);
	m_boneArrays.clear();
	u64 offset = 0;
	ProcUtil::enumerateProcessMemory(m_handle, offset, [&](char* baseAddress, u64 size, bool& bAlloc)
	{
		bAlloc = true;
		return true;
	},  [&](char* baseAddress, char* dataCpy, u64 size, bool& bFree)
	{
		for (u64 i = 0; i+12 <= size; i += 4) 
		{
			u32 boneCount = 0;
			u64 offTracked = 0;
			bool bValid = true;
		/*	if (i >= 4)
			{
				u32 val = *(u32*)(dataCpy+i-4);
				bValid  = (val == 40);
			}*/
			if (bValid)
			{
				for (u32 j = 0; j < numOffests; j++)
				{
					offTracked += offsets[j];
					if ((offTracked+12 > size) || !isBone(dataCpy + i + offTracked))
					{
						bValid = false;
						break;
					}
					boneCount++;
				}
			}
			if (bValid)
			{
				BoneArray ba;
				ba.data = (char*)malloc(12*boneCount);
				ba.numBones = boneCount;
				ba.baseAddress = baseAddress;
				ba.offset = i;
				float yLow = 10000.f;
				float yMax = 0.f;
				offTracked = 0;
				for (u32 j = 0; j < boneCount ; j++)
				{
					offTracked += offsets[j];
					Vec3 p =  *(Vec3*)(dataCpy + i + offTracked);
					if (p.z > yMax) yMax = p.z;
					if (p.z < yLow) yLow = p.z;
					memcpy(ba.data + j*12, &p, 12);
				}

				if (yLow > 50 && yMax < 1100)
				{
					bool bValidBoneArray = validateBoneArray3(ba);
					if (bValidBoneArray)
					{
					//	printf("Valid bone Array, %d bones\n", boneCount);
						m_boneArrays.emplace_back(ba);
						i += offTracked;

					//	printf("new good match found\n");
						//for (u32 j = 0; j < boneCount ; j++)
						//{
						//	Vec3 bp = *(Vec3*)(ba.data + j*12);
						//	float delta = 0.f;
						//	if (j > 0)
						//	{
						//		Vec3 bp0 = *(Vec3*)(ba.data + (j-1)*12);
						//		delta = bp.dist(bp0);
						//	}
						//	printf("%.3f %.3f %.3f %.3f\n", bp.x, bp.y, bp.z, delta);
						//}

					}
					else
					{
						free(ba.data);
					}
				}
				else
				{
					free(ba.data);
				}
			}

	/*			u32 tsize = boneCount * offsetDelta;
				BoneArray boneArray  = makeBoneArray(baseAddress, dataCpy, i, tsize, boneCount);
				bool bValidBoneArray = validateBoneArray(boneArray, offsetDelta);
				if (bValidBoneArray)
				{
					m_boneArrays.emplace_back(boneArray);
					i += tsize;
					printf("Valid bone Array, %d bones, offset %d !\n", boneCount, offsetDelta);
				}
				else
				{
					free(boneArray.data);
				}*/
			
		}
		bFree = true;
		return true;
	});
//	unique(m_boneArrays);
	m_boneArrays.sort( [] (auto& a, auto& b) { return a.numBones < b.numBones; } );
	//for (auto& ba : m_boneArrays)
	//{
	//	printf("Num bones in array %d\n", (u32) ba.numBones);
	//	for (u32 j = 0; j < ba.numBones ; j++)
	//	{
	//		Vec3 bp = *(Vec3*)(ba.data + j*12);
	//		float delta = 0.f;
	//		if (j > 0)
	//		{
	//			Vec3 bp0 = *(Vec3*)(ba.data + (j-1)*12);
	//			delta = bp.dist(bp0);
	//		}
	//		printf("%.3f %.3f %.3f %.3f\n", bp.x, bp.y, bp.z, delta);
	//	}
	//}
	printf("Num boneArrays found %lld\n", m_boneArrays.size());

}

void Scanner2::refreshBones()
{
	for (auto it = m_boneArrays.begin(); it != m_boneArrays.end();)
	{
		if (!validateBoneArray2(*it))
		{
			it = m_boneArrays.erase(it);
			continue;
		}
		else it++;
	}
	for (auto& ba : m_boneArrays)
	{
		printf("Num bones in array %d\n", (u32) ba.numBones);
	}
	printf("Num boneArrays found %lld\n", m_boneArrays.size());
}

void Scanner2::countBonesAround(u32 entryIdx)
{
	if (entryIdx >= m_entries.size())
	{
		printf("entryidx out of range\n");
		return;
	}
	u32 boneOffset = 64;
	const MemoryEntry3* e = list_at(m_entries, entryIdx);
	MemoryBlock block;
	u32 numBones = 0;
	u32 numBonesAfter = 0;
	u64 offset = e->offset - 8; // we found Z value, subtract x and y
	if (0 == ProcUtil::readSingleProcessMemoryBlock(m_handle, e->baseAddress, block))
	{
		for (i64 off = (i64)offset-boneOffset; off >= 0; off -= boneOffset)
		{
			if (isBone(block.data + off)) numBones++;
			else break;
		}
		for (i64 off = offset; off+32 <= (i64) block.size; off += boneOffset)
		{
			if (isBone(block.data + off)) numBonesAfter++;
			else break;
		}
	}
	free(block.data);
	printf("Num bones around entryIdx %d | before %d | after %d | total %d\n", entryIdx, numBones, numBonesAfter, numBones+numBonesAfter);
}

Player Scanner2::makePlayer(char* baseAddress, char* dataCpy, u64 offset)
{
	Player player;
	player.baseAddress = baseAddress;
	player.offset = offset;
	player.data = (char*)malloc(Player::Size);
	memcpy(player.data, dataCpy + offset, Player::Size);
	return player;
}

BoneArray Scanner2::makeBoneArray(char* baseAddress, char* dataCpy, u64 offset, u32 size, u32 numBones)
{
	BoneArray boneArray;
	boneArray.baseAddress = baseAddress;
	boneArray.offset = offset;
	boneArray.data = (char*)malloc(size);
	boneArray.numBones = numBones;
	memcpy(boneArray.data, dataCpy + offset, size);
	return boneArray;
}

void Scanner2::showPlayers()
{
//	refreshPlayers();
	for (auto it = m_players.begin(); it != m_players.end(); it++)
	{
		Player& player = *it;
		Vec3 p = player.getPosition();
		Vec3 f = player.getFacing();
		float dz = player.getHeadZ() - p.z;
		u32 p0 = player.padd0();
		u32 p1 = player.padd1();
		u32 p2 = player.padd2();
		printf("Pos %.3f %.3f %.3f | Fac %.3f %.3f %.3f | dZ %.3f | padd0 %d | padd1 %d | padd2 %d\n", 
			   p.x, p.y, p.z, f.x, f.y, f.z, dz, p0, p1, p2);
	}
}

void Scanner2::writeTodisk(u32 fileIdx, bool clearAfter)
{
	FILE* f;
	String path = String("temp/entries_") + std::to_string(fileIdx);
	fopen_s(&f, path.c_str(), "wb");
	if (f)
	{
		size_t numEntries = m_entries.size();
		size_t res = fwrite(&numEntries, sizeof(size_t), 1, f);
		if (res == 1)
		{
			for (auto& e : m_entries)
			{
				res = fwrite(&e, sizeof(MemoryEntry3), 1, f);
				if (res != 1) 
				{
					printf("File write error.\n");
					break;
				}
			}
		}
		fflush(f);
		fclose(f);
	}
	else perror("File open failed. ");
	if (clearAfter) m_entries.clear();
}

u32 Scanner2::readFromDisk(u32 fileIdx, bool readContent, bool clearBefore)
{
	size_t numEntries = 0;
	if (clearBefore) m_entries.clear();
	FILE* f;
	String path = String("temp/entries_") + std::to_string(fileIdx);
	fopen_s(&f, path.c_str(), "rb");
	if (f)
	{
		size_t res = fread(&numEntries, sizeof(size_t), 1, f);
		if (res == 1)
		{
			if (readContent)
			{
				for (size_t i=0; i<numEntries; i++)
				{
					MemoryEntry3 entry;
					auto res = fread(&entry, sizeof(MemoryEntry3), 1, f);
					if (res == 1)
					{
						m_entries.emplace_back(entry);
					}
					else 
					{
						printf("File read error.\n");
						break;
					}
				}
			}
		}
		fflush(f);
		fclose(f);
	}
	else perror("File read failed." );
	return (u32) numEntries;
}

void Scanner2::forEachEntry(const std::function<bool (MemoryEntry3&, MemoryBlock&)>& cb)
{
	if (!m_handle) return;
	assert(cb);
	MemoryBlock block;
	memset(&block, 0, sizeof(block));
	u32 numFiles = m_numFiles;
	if (m_inMemory) numFiles = 1;
	for (u32 fileIdx = 0; fileIdx < numFiles; fileIdx++)
	{
		if (!m_inMemory) readFromDisk(fileIdx, true, true); // populates to entries
		for (auto it = m_entries.begin(); it != m_entries.end();)
		{
			MemoryEntry3& e = *it;
			if (block.baseAddress != e.baseAddress || block.error != 0 || e.offset + 4 > block.size)
			{ // free cache
				free(block.data);
				i32 err = ProcUtil::readSingleProcessMemoryBlock(m_handle, e.baseAddress, block);
				if (err != 0 || e.baseAddress != block.baseAddress || block.error != 0 || e.offset + 4 > block.size)
				{
					if (err == -2) it = m_entries.erase(it); // freed memory
					else it++;
					continue;
				}
			}

			bool bErase = cb(e, block);
			if (bErase)
			{
				it = m_entries.erase(it);
			}
			else it++;
		}
		// write updated content (delete or value update) back
		if (!m_inMemory) writeTodisk(fileIdx, true);
	}
	free(block.data);
}

void Scanner2::printEntry(MemoryEntry3& e, MemoryBlock& block, u32 idx, u32 diff)
{
	char* ptr   = block.data + e.offset;
	bool asPtr  = e.offset % 8 == 0;
	char* oriPtr = (char*) block.baseAddress + e.offset;
	printData(idx, diff, ptr, asPtr, nullptr, oriPtr, e.offset, block.size);
}

void Scanner2::printData(u32 idx, i32 diff, char* ptr, bool bAsPtr, FILE* fl, char* oriPtr, u64 ofsInBlock, u64 blockSize)
{
	float f     = *(float*)ptr;
	i32 i	    = *(i32*)ptr;
	char* asPtr = nullptr;
	if (bAsPtr) asPtr = *(char**)ptr;
	char str[] = { *ptr, *(ptr+1), *(ptr+2), *(ptr+3), '\0' };

	//if (!isValidInt(i))  i = 0;
	//if (!insideWorld(f)) f = 0;
	//if (asPtr > maxPtr) asPtr = nullptr;

	if (i == 0) i = 1111111111;
	if (f == 0 || f < -1e20 || f > 1e20) f = 1.11111111f;

	printf("%p\t %p\t %d\t %d\t %d\t %f\t %p\t %s\t %lld\t %lld\n", oriPtr, ptr, diff, idx, i, f, asPtr, str, ofsInBlock, blockSize);
	if (fl)
	{
		fprintf_s(fl, "%p\t %p\t %d\t %d\t %d\t %f\t %p\t %s\t %lld\t %lld\n", oriPtr, ptr, diff, idx, i, f, asPtr, str, ofsInBlock, blockSize);
	}
}

bool Scanner2::nearZero(float f) const
{
	return f >= -0.0001f && f <= 0.0001f;
	//return (f > -20.101f && f < 20.101f);
}

bool Scanner2::insideWorld(float f) const
{
	return (f > -10000.f && f < 10000.f) && !isIntegral(f) && !nearZero(f) && !isnan(f);
	//return (f > -10000.f && f < 10000.f) && !isIntegral(f) && !nearZero(f) && !isnan(f);
}

bool Scanner2::validFloat(float f) const
{
	// cww2 specific y at point du hoc
	//return f >= 140.f && f < 170.f && !isIntegral(f);
	return !nearZero(f) && insideWorld(f) && !isIntegral(f);
	//return insideWorld(f);
}

bool Scanner2::validUp(float f) const
{
	return (f >= 50.f && f <= 1100.f) && !isIntegral(f) && !nearZero(f) && !isnan(f);
	//return f >= 50 && f <= 1100.f && !isIntegral(f) && !nearZero(f) && !isnan(f);
}

bool Scanner2::isIntegral(float f) const
{
	return (float)i32(f) == f;
}

bool Scanner2::isValidInt(i32 i) const
{
	return i >= -1000 && i <= 1000;
}

bool Scanner2::validPosition(const Vec3& pos) const
{
	return (insideWorld(pos.x) && insideWorld(pos.y) && 
			validUp(pos.z));

	//for (auto& f : pos.m)
	//{
	//	if (!insideWorld(f) || nearZero(f))
	//		return false;
	//}
	//if (!validUp(pos.z)) return false;
	//return true;
}

bool Scanner2::validVector(const Vec3& facing) const
{
	float l = facing.length();
	if ( l >= 0.95f && l <= 1.05f &&
		 !(facing.x == 0.f || facing.y == 0.f || facing.z == 0.f) )
	{
		return true;
	}
	return false;
	//return fabs(facing.length()-1.f) < 0.001f;
}

bool Scanner2::isPlayer(char* p) const
{
	u32 off = 220;
	Vec3 rv1	  = *(Vec3*)(p-220 + off);
	Vec3 rv0	  = *(Vec3*)(p-204 + off);
	Vec3 rv2	  = *(Vec3*)(p+120 + off);
	u32 padd0	  = *(u32*)(p + off);
	Vec3 position = *(Vec3*)(p+4 + off);
	u32 padd1	  = *(u32*)(p+16 + off);
	float fpadd1  = *(float*)(p+16 + off);
	Vec3 facing   = *(Vec3*)(p+20 + off);
	u32 padd2	  = *(u32*)(p+32 + off);
	Vec3 headPos  = *(Vec3*)(p+36 + off);
	Vec3 upVec	  = *(Vec3*)(p+72 + off);
	float dz = headPos.z - position.z;

	if (position.z >= 1 && position.z < 900 &&
		validPosition(position) &&
		validVector(facing) &&
		dz >= 300 && dz < 800 &&
		padd2 > 700 && padd2 < 1024
		//padd0 == 0 &&
		//padd2 == 0 &&w
		//fpadd1 >= 0.24f && fpadd1 <= 0.26f
//		validVector(facing) &&
//		validVector(upVec)
		//validVector(rv0) &&
		//validVector(rv1)/* &&
//		dz >= 300 && dz < 800*/
		// &&
		/*validVector(rv2)*/)
	{
		return true;
	}
	return false;
}

bool Scanner2::isBone(char* data) const
{
	Vec3 p0 = *(Vec3*)(data);
	//Vec3 p1 = *(Vec3*)(data + 16);
	return validPosition(p0);// && validPosition(p1);
}

Null::u32 Scanner2::playerSize() const
{
	return Player::Size;
}

bool Scanner2::validateBoneArray(BoneArray& boneArray, u32 boneOffset) const
{
	// 1 meter is 58.3333
	float m2u = 40.33333f;
	float u2m = 1.f/m2u;
	float maxL = 4.f * m2u;
	float minL = .5f * m2u;
	float halfcmu = 1.5f/100 * m2u;
	for (u32 i=0; i<boneArray.numBones; i++)
	{
		Vec3 b1 = *(Vec3*)( boneArray.data + i*boneOffset );
		u32 numClose = 0;
		u32 numBoneClose = 0;
		for (u32 j = 0; j < boneArray.numBones ; j++)
		{
			if (i == j) continue;
			
			Vec3 b2 = *(Vec3*)( boneArray.data + j*boneOffset );
			float tempL = b1.dist(b2);
			if (tempL > maxL) return false;
			if (tempL < halfcmu*3) return false;

			float lx = fabs(b1.x - b2.x);
			float ly = fabs(b1.y - b2.y);
			float lz = fabs(b1.z - b2.z);

			if (lx < halfcmu || ly < halfcmu || lz < halfcmu) 
			{
				numBoneClose++;
				if (numBoneClose == 12) return false;
			}

			if (tempL < minL)
			{
				numClose++;
			}
		}
		if (numClose == 0)
		{
			return false;
		}
	}
	return true;
}

bool Scanner2::validateBoneArray2(BoneArray& boneArray) const
{
	const u32 offsets [] = {
		///*48,*/ 0, 48, 1374752, 
		0,
		16, 48, 
		16, 48,
		16, 48,
		16, 48,
		// 4
		16, 48,
		16, 48,
		16, 48,
		16, 48,
		// 8
		16, 48,
		16, 48,
		16, 
		// 11
		//	5776,
		//32, 32, 32,// 32, 32,
		//200096,
		//48, 48,
		//320,
		//48, 48,
		//320,
		//48, 48,
		//320,
		//48, 48,
		//320,
		//48, 48,
		//320,
		//48,
		//48//,
		//	13071536
	};
	u32 numOffests = sizeof(offsets) / sizeof(u32);
	u64 offTracked = 0;
	for (u32 j = 0; j < boneArray.numBones ; j++)
	{
		offTracked += offsets[j];
		MemoryBlock block;
		if (0 == ProcUtil::readProcessMemory(m_handle, boneArray.baseAddress + boneArray.offset + offTracked, 12, block))
		{
			Vec3 vold = *(Vec3*)( boneArray.data + j*12 );
			Vec3 vnew = *(Vec3*)( block.data );
			if (!validPosition(vold) || !validPosition(vnew)) return false;
			if (vold == vnew) return false; // must change (always animating, also in idle)
		}
		free(block.data);
	}

	return true;
}

bool Scanner2::validateBoneArray3(BoneArray& boneArray)
{
	for (u32 i=0; i<boneArray.numBones; i++)
	{
		Vec3 b1 = *(Vec3*)( boneArray.data + i*12 );
		u32 numClose = 0;
		for (u32 j = 0; j < boneArray.numBones ; j++)
		{
			if (i == j) continue;

			Vec3 b2 = *(Vec3*)( boneArray.data + j*12 );

			//float zdiff = fabs(b2.z - b1.z);
			//if (zdiff > 25.f)
			//	return false;
			//if (zdiff < 0.0001f)
			//{
			//	numClose++;
			//	if (numClose == 4)
			//		return false;
			//}

			float tempL = b1.dist(b2);
			if (tempL > 50.f)  return false;
			if (tempL < 0.00001f)
			{
				numClose++;
				if (numClose == 3) 
				{
					return false;
				}
			}
		}
	}
	return true;

	//for (u32 i=0; i<boneArray.numBones; i++)
	//{
	//	Vec3 b1 = *(Vec3*)( boneArray.data + i*sizeof(Vec3) );
	//	for (u32 j = 0; j < boneArray.numBones ; j++)
	//	{
	//		if (i == j) continue;
	//		Vec3 b2 = *(Vec3*)( boneArray.data + j*sizeof(Vec3) );
	//		float tempL = b1.dist(b2);
	//		if (tempL > 50 || tempL < 0.1f) return false;
	//	}
	//}
	//return true;
}
