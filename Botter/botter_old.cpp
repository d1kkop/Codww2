

//
//constexpr u32 g_boneOffsets [] = {
//	///*48,*/ 0, 48, 1374752, 
//	0,
//	16, 160, 
//	16, 160,
//	16, 160,
//	16, 160,
//	// 4
//	16, 160,
//	16, 
//	// 11
//	/////*48,*/ 0, 48, 1374752, 
//	0,
//	16, 48, 
//	16, 48,
//	16, 48,
//	16, 48,
//	// 4
//	16, 48,
//	16, 48,
//	16, 48,
//	16, 48,
//	// 8
//	16, 48,
//	16, 48,
//	16, 
//	// 11
//	//	5776, 32, 32, 32, 32, 32, 200096, 48, 48, 320, 48, 48, 320, 48, 48, 320, 48, 48, 320, 48, 48, 320, 48, 48,	13071536
//};



//u64 ofsInBlockStart =
// 	u32 scanIdx = m_scanner.newGenericScan(64*g_numBones + 8 + 128 /*g_absBoneOffsets[g_numBones-1]+64*/, 4, 0, -1, ValueType::Memory,
// 										   [&](const char* p, u64 offset, u64 offInBlock, u64 blockSize)
// 	{
// 		bool bKeepEntry = false;
// 		u32 typeId = 0;
// 		//if ( blockSize > 1800000000 )
// 		{
// 			bKeepEntry = isValidBoneArray2(p, nullptr, typeId);
// 			if ( bKeepEntry )
// 			{
// 				if (blockSize != prevBlockSize)
// 				{
// 					prevBlockSize = blockSize;
// 					printf("blockSize %lld\n", blockSize);
// 				}
// 				if ( offset < lowOffset ) lowOffset = offset;
// 				if ( offset > highOffset) highOffset = offset;
// 			}
// 		}
// 		return bKeepEntry;
// 	},
// 		&preScanCallback
// 	);




//m_boneArrayScan->removeDuplicates(m_boneArrayScan->getValueType(), [&](const MemEntry &a, const MemEntry& b)
//{
//	return false;

//	Player pa, pb;
//	pa.boneData = a.value.ptr;
//	pb.boneData = b.value.ptr;

//	if ( &a == &b ) return false;

//	Vec3 ca(0,0,0);
//	Vec3 cb(0,0,0);
//	for (u32 i = 0; i < pa.numBones() ; i++)
//	{
//		ca += pa.getBone(i);
//		cb += pb.getBone(i);
//	}
//	float denom = 1.f / pa.numBones();
//	ca *= denom;
//	cb *= denom;

//	float dist = ca.dist(cb);
//	return (dist < 35); // then equal
////	return false;
//});

//u32 i=0;
//m_boneArrayScan->forEachEntry([&](MemEntry& e)
//{
//	Player p;
//	p.boneData = e.value.ptr;
//	printf("Bones for player %d----\n", i++);
//	for (u32 j = 0; j < p.numBones() ; j++)
//	{
//		Vec3 b = p.getBone(j);
//		printf("Bone %d | %.3f %.3f %.3f\n", j, b.x, b.y, b.z);
//	}
//	return true;
//});

//printf("Begin show entries...----\n\n");
//u32 numEntriesChanged = 0;
//m_scanner.updateEntries(m_boneArrayScan->getId(), [&](const char* newData, const char* oldData, u32& typeId)
//{
//	for (u32 i = 0; i < g_numBones ; i++)
//	{
//		if (!mem_equal(newData + i*64, oldData + i*64, 12) || 
//			!mem_equal(newData + i*64 + 16, oldData + i*64 + 16, 12))
//		{
//			numEntriesChanged++;
//			break;
//		}
//	}
//	
//	return true;
//});
//printf("Num entries changed %d\n", numEntriesChanged);

//u32 i=0;
//MemoryBlock cache;
//mem_zero(&cache, sizeof(MemoryBlock));
//m_boneArrayScan->forEachEntry([&](MemEntry& e)
//{
//	
//	m_scanner.showAround(m_boneArrayScan->getId(), i++, 0, 2, false, cache);
//	return true;
//});
//mem_free(cache.data);
//m_scanner.clearPtrList(m_boneArrayScan->getId());

//m_scanner.filterGenericScan( m_boneArrayScan->getId(), [&](const char* newData, const char* oldData, u64 ofsInBlock, u64 blockSize, u32& typeId)
//{
//	bool bKeepData = isValidBoneArray(newData, oldData, typeId);
//	return bKeepData;
//});

//u64 numEntries = m_scanner.getNumEntries(m_boneArrayScan->getId(), 1);
//if (numEntries > 1000)
//{
//	m_boneArrayState = MemoryState::Find;
//	printf("Too many valid bone arrays found %lld, back to filtering only...\n", numEntries);
//	return;
//}


//// updata data
//bool bValid = m_scanner.updateEntries(m_boneArrayScan->getId(), [&](const char* newData, const char* oldData, u32& typeId)
//{
//	bool bKeep = isValidBoneArray(newData, oldData, typeId);
//	return bKeep;
//});

//if (m_boneArrayScan->numEntries() == 0)
//{
//	m_boneArrayState = MemoryState::Find;
//	printf("No bone arrays anymore after updating entries, refinding..\n");
//	return;
//}

//if (!bValid)
//{
//	m_boneArrayState = MemoryState::Find;
//	printf("One of the bone arrays no longer valid, refinding..\n");
//	return;
//}


//bool Botter::isValidCamera(const char* p) const
//{
//	//Vec3 rv0	  = *(Vec3*)(p);
//	//Vec3 rv1	  = *(Vec3*)(p+16);
//	//Vec3 rv2	  = *(Vec3*)(p+340);
//	//u32 padd0	  = *(u32*)(p+220);
//	//Vec3 position = *(Vec3*)(p+224);
//	//float fpadd1  = *(float*)(p+236);
//	//Vec3 facing   = *(Vec3*)(p+240);
//	//u32 padd2		= *(u32*)(p+252);
//	//Vec3 headPos  = *(Vec3*)(p+256);
//	//Vec3 posRVec  = *(Vec3*)(p+272); // possible rVec
//	//Vec3 upVec	= *(Vec3*)(p+292);
//	//float dz		= headPos.z - position.z;
//
//	Vec3 position = *(Vec3*)(p);
//	Vec3 facing   = *(Vec3*)(p+16);
//	Vec3 headPos  = *(Vec3*)(p+32);
//	Vec3 upVec	  = *(Vec3*)(p+68);
//	float alw4	  = *(float*)(p+104);
//	//float dz = headPos.z - position.z;
//
//	if (validPosition(position) &&
//		validUp(position.z) && 
//		validVector(facing) &&
//		alw4 >= 3.99f && alw4 <= 4.01f &&
//		//validPosition(headPos) &&
//		validVector(upVec) //&&
//						   //	validVector(rv0) &&
//						   //	validVector(rv1) &&
//						   //	validVector(rv2) //&&
//						   //dz >= 200 && dz < 900)
//		)
//	{
//		//		printf("Found Camera:\n");
//		//		printCameraStats(position, facing, upVec, headPos);
//		return true;
//	}
//
//	return false;
//}


//bool Botter::isValidPlayerPos(const char* p) const
//{
//	Vec3 vec2_0		= *(Vec3*)(p);
//	Vec3 vec2_1		= *(Vec3*)(p+16);
//	Vec3 pos		= *(Vec3*)(p+96);
//	Vec3 pos_2		= *(Vec3*)(p+112);
//	Vec3 vec3_0		= *(Vec3*)(p+416);
//
//	vec2_0.z = 0;
//	vec2_1.z = 0;
//
//	if (validPosition(pos) &&
//		validUp(pos.z) && 
//		validVector(vec2_0) &&
//		validVector(vec2_1) &&
//		pos == pos_2// &&
//					//		validVector(vec3_0)
//		)
//	{
//		return true;
//	}
//
//	return false;
//}


//bool Botter::isValidBoneArray(const char* newData, const char* oldData, u32& typeId) const
//{
//	typeId = 1;
//
//	float zMin = FLT_MAX;
//	float zMax = FLT_MIN;
//	Vec3 avg;
//
//	for (u32 i = 0; i < g_numBones ; i++)
//	{
//		if (g_boneOffsets[i] == 48)
//		{
//			u32 zero = *(u32*)(newData + g_absBoneOffsets[i] - 4);
//			u32 zero2 = *(u32*)(newData + g_absBoneOffsets[i] - 8);
//			if (!(zero >= 0 && zero <= 200) || !(zero2 >= 0 && zero2 <= 200))
//				return false;
//		}
//
//		Vec3 v = *(Vec3*)(newData + g_absBoneOffsets[i]);
//		if (!validPosition(v)) return false;
//
//		if (v.z < zMin) zMin = v.z;
//		if (v.z > zMax) zMax = v.z;
//
//		avg += v;
//
//		// check integrity of bone structure
//		u32 numClose = 0;
//		for (u32 j=i+1; j<g_numBones; j++)
//		{
//			Vec3 b = *(Vec3*)(newData + g_absBoneOffsets[j]);
//
//			float zDist = fabs(b.z - v.z);
//			if (zDist > 40.f) return false;
//			if (zDist < 0.001f)
//			{
//				numClose++;
//				if (numClose == 6) return false;
//			}
//		}
//	}
//
//	float zSpan = fabs(zMax - zMin);
//	if (zSpan < 2.f || zSpan > 40.f )return false;
//
//	avg *= (1.f / g_numBones);
//
//	// made it so far, now for each bone see how far from normal
//	if ( avg.length() >= 1.f )
//	{
//		const float maxDeviation = 175;
//		for (u32 i = 0; i < g_numBones ; i++)
//		{
//			Vec3 v = *(Vec3*)(newData + g_absBoneOffsets[i]);
//			for (u32 j=0; j<3; j++)
//			{
//				float f = fabs( v.m[j] - avg.m[j] );
//				if (f > maxDeviation) return false;
//			}
//		}
//	}
//
//	if (oldData)
//	{
//		//// compute avg delta
//		//float avgDelta = 0.f;
//		//for (u32 i = 0; i < g_numBones ; i++)
//		//{
//		//	Vec3 bnew = *(Vec3*)(newData + g_absBoneOffsets[i]);
//		//	Vec3 bold = *(Vec3*)(oldData + g_absBoneOffsets[i]);
//		//	float d = bnew.dist(bold);
//		//	avgDelta += d;
//		//}
//
//		//// compute delta again, if deviates more than threshold, consider invalid bone structure
//		//if (avgDelta > 1.f) 
//		//{
//		//	avgDelta /= g_numBones;
//		//	for (u32 i = 0; i < g_numBones ; i++)
//		//	{
//		//		Vec3 bnew = *(Vec3*)(newData + g_absBoneOffsets[i]);
//		//		Vec3 bold = *(Vec3*)(oldData + g_absBoneOffsets[i]);
//		//		float d = bnew.dist(bold);
//		//		d /= avgDelta;
//		//		if ( d < 1.f ) d = 1.f - d;
//		//		else d -= 1.f;
//		//		// threshold 25%
//		//		if ( d > .25f )
//		//			return false;
//		//	}
//		//}
//
//		//typeId = 0;
//		//u32 numUnequal = 0;
//		//for (u32 i = 0; i < g_numBones ; i++)
//		//{
//		//	Vec3 v = *(Vec3*)(newData + g_absBoneOffsets[i]);
//		//	Vec3 b = *(Vec3*)(oldData + g_absBoneOffsets[i]);
//		//	if (!(v == b))
//		//	{
//		//		numUnequal++;
//		//	}
//		//}
//
//		//if ( numUnequal > 5 ) typeId = 1;
//	}
//
//	// keep data
//	return true;
//}
//
//bool Botter::isValidBoneArray2(const char* newData, const char* oldData, u32& typeId) const
//{
//	for (u32 i = 0; i < g_numBones ; i++)
//	{
//		//if (i == 0)
//		//{
//		//	u32 fourty = *(u32*)(newData + 4);
//		////	if (fourty >= 40 && fourty <= 56)
//		//	if ( fourty != 72 )
//		//	{
//		//		if (m_reportBoneMatchFail) printf("Not 40\n");
//		//		return false;
//		//	}
//		//}
//
//		if (i != 0)
//		{
//			u32 zero  = *(u32*)(newData + i*64 + 0);
//			u32 zero1 = *(u32*)(newData + i*64 + 4);
//			if (zero != 0 || zero1 != 0)
//			{
//				if (m_reportBoneMatchFail) printf("Not zero\n");
//				return false;
//			}
//		}
//
//		Vec3 v  = *(Vec3*)(newData + i*64 + 8);
//		Vec3 v1 = *(Vec3*)(newData + i*64 + 16 + 8);
//		if (!validPosition(v) || !validPosition(v1))
//		{
//			if (m_reportBoneMatchFail) printf("Not position\n");
//			return false;
//		}
//	}
//	typeId = 1;
//	return true;
//}



////u64 t = *(u64*)(p + 0) ;
////if (t!= 0xFFFFFFFF) return false;

////t = *(u64*)(p + 8);
////if ( != 0x47CB2598) return false;
//printf("Is valid bone array check..\n");

//// alive ?
//u32 t = *(u32*)(p + 0);
//if (!(t == 1 || t == 2 || t == 0 || t == 3)) return false;

////// team?
//t = *(u32*)(p + 4);
//if (!(t == 1 || t == 0x10101 || t == 2 || t == 0x10102 || t == 3 || t == 0x10103)) return false;

//t = *(u32*)(p + 12);
//if (t != 0x3F800000) return false;

//Vec3 pos = *(Vec3*)(p + 44);
//if (!validPosition(pos)) return false;

//return true;

//////u32 knum = 32;
//////u32 k = *(u32*)(newData);
//////for (u32 i = 0; i < knum ; i++)
//////{
//////	if (*(u32*)(newData + i*4) != 0) return false;
//////}
////////if (k != 2) return false;


//// dbg self
//{
//	float closest  = FLT_MAX;
//	GamePlayer* target = nullptr;
//	for (auto& p : m_gamePlayers)
//	{
//		Vec3 dir   = p.getPosition() - ownPos;
//		float dist = dir.length() * U2M; // roughly to meters
//		dir.normalize();
//		float kDot = viewDir.dot(dir);

//		//	printf("Dist in M %.3f\n", dist);
//		if (dist < MinDst  &&
//			dist < closest)  // find closest in aim range
//		{
//			target  = &p;
//			closest = dist;
//		}
//	}
//	if (target) 
//	{
//		Vec3 r = (target->bones[5]-ownPos);
//		r.normalize();
//		printf("Dir to Self %.3f %.3f %.3f \n", r.x, r.y, r.z);
//	}
//}


//printf("Vd %.3f %.3f %.3f | uD %.3f %.3f %.3f | sD %.3f %.3f %.3f | dots : %.3f %.3f %.3f \n",
//	   viewDir.x, viewDir.y, viewDir.z,
//	   upDir.x, upDir.y, upDir.z,
//	   sideDir.x, sideDir.y, sideDir.z,
//	   viewDir.dot(upDir),
//	   viewDir.dot(sideDir),
//	   upDir.dot(sideDir));

//printf( "--> HasTarget dist %.3fM | kDot %.3f <-- \n", dist, kDot );
//for (auto& b : m_target->bones)
//	printf("Bone %.3f %.3f %.3f \n", b.x, b.y, b.z );



//const MemEntry* entry = m_gameBoneArrayScan->firstEntry();
//Vec3* posPtr = (Vec3*) ( entry->value.ptr + BoneOffset );
//
//while (true)
//{
//	if ( (nearZero(posPtr->x) && nearZero(posPtr->y) && nearZero(posPtr->z)) || 
//		 !validPosition(*posPtr) )
//	{
//		if (i < 8) // restart search
//		{
//			m_scanner.deleteScan(m_gameBoneArrayScan->getId());
//			m_gameBoneArrayScan = nullptr;
//			m_gamePlayers.clear();
//			m_boneArrayState = MemoryState::Find;
//			return;
//		}
//		break;
//	}

//	if (i >= m_gamePlayers.size())
//		m_gamePlayers.emplace_back(GamePlayer());

//	GamePlayer& gp = m_gamePlayers[i];
//	gp.setPosition( *posPtr );	

//	posPtr += 1;
//	i++;
//}

//m_gamePlayers.resize(i);

//// 	const float headZ = AimZ;
//	if (PositionTs == 0) return pos;
//	float tNow = to_seconds( time_now() );
//	float dt = tNow - PositionTs;
//	if ( dt > MaxExtrTime ) // max of 1 sec extrapolation
//		dt = MaxExtrTime;
//	//printf("Vel %.3f %.3f %.3f Dt %.3f\n", vel.x, vel.y, vel.z, dt);
//	Vec3 vel2 = vel;
//	vel2.z *= ZExtrFac;
//	return pos + Vec3(0,0,headZ) + vel2 * dt * ExtrMultp;


//float newTs = to_seconds( time_now() );
//float dt = (newTs - PositionTs);
//if (!(p == pos) || dt > MaxExtrTime)
//{
//	vel = (p - pos) * (1.f / dt);
//	pos = p;
//	PositionTs  = newTs;
//}

//m_gameBoneArrayScan->forEachEntry( [&](MemEntry& entry)
//{
//	Bone bone = *(Bone*)(entry.value.ptr);
//	Vec3 pos  = bone.pos;
//	bool bWasPlaced = false;
//	for (auto& barray : m_boneArrays)
//	{
//		float dist = barray.getPosition().dist(pos);
//		if ( dist*U2M < BoneDistThreshold )
//		{
//			EntryVelocity ev;
//			ev.e = &entry;
//			barray.entries.emplace_back(ev);
//			bWasPlaced = true;
//			break;
//		}
//	}
//	// start new group
//	if (!bWasPlaced)
//	{
//		BoneArray ba;
//		ba.id = m_uniqueBoneArrayId++;
//		EntryVelocity ev;
//		ev.e = &entry;
//		ba.entries.emplace_back(ev);
//		m_boneArrays.emplace_back(ba);
//	}
//	return true;
//});




// ------------------- BoneArray ----------------------------------------------------------------------------------------------------
//
//void BoneArray::update()
//{
//	 if already valid, skip validation anyalysis
//	if (valid) return;
//
//	u32 numValid = 0;
//	bool canBeValid = true;
//	for (auto it = entries.begin(); it != entries.end(); )
//	{
//		EntryVelocity& ev = *it;
//		ev.vel.update(ev.e);
//		float velMs = ev.vel.velocity().length() * U2M;
//		if ( velMs > VelMaxValid ) // invalid (remove 'teleporting' bones)
//		{
//			NumInvalidMovesDetected++;
//			if ( NumInvalidMovesDetected > NumInvalidMeasurementsBeforeRemove )
//			{
//				it = entries.erase(it);
//			}
//			canBeValid = false;
//		}
//		else
//		{
//			 check if considered moving
//			if ( velMs > VelMoveThreshold ) numValid++;
//			it++;
//		}
//	}
//
//	if (canBeValid && numValid == (u32)entries.size() && numValid != 0)
//	{
//		float tNow = to_seconds(time_now());
//		if (!IsMoving)
//		{
//			IsMoving = true;
//			StartedMovingTs = tNow;
//		}
//		else if ( tNow - StartedMovingTs > ConsiderMovingTimeSpan )
//		{
//			NumMovesDetected += 1;
//			 reset analysis
//			IsMoving = false;
//			 check valid
//			if (NumMovesDetected >= NumMeasurementsBeforeValid)
//			{
//				valid = true;
//				printf("Bone array with %d bones has become valid!\n", (u32)entries.size());
//			}
//		}
//	}
//}
//
//Vec3 BoneArray::computeDirTo(const Vec3& pos, float& distOut) const
//{
//	float dist = getPosition().dist(pos);
//	Vec3 dir   = getPosition() - pos;
//	distOut = dir.length();
//	dir.normalize();
//	return dir;
//}
//
//Vec3 BoneArray::getPosition() const
//{ 
//	if ( !entries.empty() )
//	{
//		return ((Bone*)entries.front().e->value.ptr)->pos;
//	}
//	return Vec3();
//}
//
//Vec3 BoneArray::getVelocity() const
//{
//	if ( !entries.empty() )
//	{
//		return entries.front().vel.velocity();
//	}
//	return Vec3();
//}
//
//bool BoneArray::isValid() const
//{
//	return valid;
//}
//
//
//const char* BoneArray::getAddress() const
//{
//	if ( !entries.empty() )
//	{
//		return entries.front().e->baseAddress + entries.front().e->offset;
//	}
//	return nullptr;
//}
//
//Vec3 BoneArray::getBonePosition(u32 boneIdx) const
//{
//	if ( !entries.empty() )
//	{
//		return ((Bone*)entries[boneIdx].e->value.ptr)->pos;
//	}
//	return Vec3();
//}


//// split bone arrays if data is invalid among a single bone array
//Array<BoneArray> newArrays;
//for (auto& barray : m_boneArrays)
//{
//	Vec3 pos = barray.getPosition();
//	for (auto it = barray.entries.begin(); it != barray.entries.end(); )
//	{
//		EntryVelocity& ev = *it;
//		const MemEntry* e = ev.e;
//		Bone bone = *(Bone*)(e->value.ptr);
//		Vec3 b = bone.pos;
//		float dist = pos.dist(b);
//		if ( dist*U2M > BoneDistThreshold ) // break it
//		{
//			bool bWasPlaced = false;
//			for (auto& narray : newArrays)
//			{
//				dist = narray.getPosition().dist(b);
//				if ( dist*U2M <= BoneDistThreshold ) // found new existing
//				{
//					bWasPlaced = true;
//					narray.entries.emplace_back(ev);
//					break;
//				}
//			}
//			if (!bWasPlaced)
//			{
//				BoneArray ba;
//				ba.id = m_uniqueBoneArrayId++;
//				ba.entries.emplace_back(ev);
//				newArrays.emplace_back(ba);
//			}
//			it = barray.entries.erase(it);
//		}
//		else it++;
//	}
//}

//// merge new groups to olds groups
//for ( auto& barray : newArrays )
//{
//	m_boneArrays.emplace_back(barray);
//}
//newArrays.clear();
//newArrays.shrink_to_fit();

//// remove empty entries
//for (auto it = m_boneArrays.begin(); it != m_boneArrays.end();)
//{
//	BoneArray& ba = *it;
//	if ( ba.entries.empty() ) it = m_boneArrays.erase(it);
//	else it++;
//}


//struct EntryVelocity
//{
//	const MemEntry* e;
//	Velocity vel;
//};

//
//void Botter::postFilterBoneArrays()
//{
//	if (!m_boneArrayScan)
//	{
//		m_boneArrayState = MemoryState::Find;
//		return;
//	}
//
//	// sort on dist to origin
//	m_scanner.sortEntries(m_boneArrayScan->getId(), [&](const char* l, const char* r)
//	{
//		Bone a = *(Bone*)(l); 
//		Bone b = *(Bone*)(r);
//		return a.pos.length() < b.pos.length();
//	});
//
//	//Bone prev;
//	m_scanner.updateEntries(m_boneArrayScan->getId(), [&](const char* p, u64 ofsInBlock, const char* newData, const char* oldData, u32& typeId)
//	{
//		//	u32 res = validBone(prev, *(Bone*)(newData), *(Bone*)(oldData), 25);
//		//	checkSpecific( res, p+ofsInBlock );
//		u32 res = isValidBoneArray(newData);
//		if ( res != 0 )
//		{
//			Bone* b = (Bone*)newData;
//			printf("Error %d pos %f %f %f %d | %d %d %d | %d %d \n", res, b->pos.x, b->pos.y, b->pos.z, validPosition(b->pos), 
//				   insideWorld(b->pos.x), insideWorld(b->pos.y), insideWorld(b->pos.z), isIntegral(b->pos.x), zOrNorm(b->pos.x));
//		}
//		return res == 0;
//	});
//
//	u32 numEntries = m_boneArrayScan->numEntries();
//	printf("Num bone arrays after sorted post filter %d\n", numEntries);
//
//	if ( numEntries < 15000 && numEntries > 0 )
//	{
//		printf("Filtered bone array progress enough. Starting update..\n");
//		std::lock_guard<std::mutex> lk(m_boneArrayMutex);
//		m_boneArrayState = MemoryState::Wait;
//		if (m_gameBoneArrayScan) m_scanner.deleteScan(m_gameBoneArrayScan->getId());
//		m_gameBoneArrayScan = m_boneArrayScan;
//		m_boneArrayScan = nullptr;
//	}
//
//	if ( numEntries == 0 )
//	{
//		printf("WARNING: No bone arrays found after filter..\n");
//		m_boneArrayState = MemoryState::Find;
//	}
//}


//
//// now sort based on dist to camera
//Vec3 camP = m_gameCam.getHeadPos();
//m_scanner.sortEntries(m_gameBoneArrayScan->getId(), [&](const char* l, const char* r)
//{
//	Bone a = *(Bone*)(l); 
//	Bone b = *(Bone*)(r);
//	return Vec3(a.pos).dist(camP) < Vec3(b.pos).dist(camP);
//});


//
//std::sort( m_boneArrays.begin(), m_boneArrays.end(), [&](auto& l, auto& r)
//{
//	return  l->getPosition().dist(m_gameCam.getHeadPos()) > 
//		r->getPosition().dist(m_gameCam.getHeadPos());
//	//	return l->getAddress() > r->getAddress();
//	//return l.entries.size() > r.entries.size();
//});

//
//
//void Botter::checkSpecific(i32 iRes, const char* ptr)
//{
//	if ( iRes != 0 && (u64)ptr == CheckSpecific )
//	{
//		printf("Specifc match FAILED (Error %d)\n", iRes);
//	}
//}


//// minimum of 8 players always
//if ( !m_haveFoundPlayers && m_gameBoneArrayScan->numEntries() >= MiniumNumBoneArrays )
//{
//	m_haveFoundPlayers = true;
//	log(str_format("Found sufficient player count (%d) for botting.. Yee!\n", m_gameBoneArrayScan->numEntries()));
//}

//// if have found players but has become less than 8, restart
//if ( m_gameBoneArrayScan->numEntries() < MiniumNumBoneArrays )
//{
//	log(str_format("WARNING insufficient player count (%d) detected..\n", m_gameBoneArrayScan->numEntries()));
//	std::lock_guard<std::mutex> lk(m_boneArrayMutex);
//	m_scanner.deleteScan( m_gameBoneArrayScan->getId() );
//	m_gameBoneArrayScan = nullptr;
//	// restart
//	assert ( m_boneArrayState == MemoryState::Wait );
//	for (auto& ba : m_boneArrays ) delete ba;
//	m_boneArrays.clear();
//	m_hasBuiltBoneArrays = false;
//	m_haveFoundPlayers = false;
//	m_boneArrayState = MemoryState::Find;
//}



//static const char* g_anchor = nullptr;
//static const u32 searchArea = 0xFFFFFF * 2;

//if ( !g_anchor )
//{
//	MemoryBlock block;
//	mem_zero(&block, sizeof(block));
//	if (ProcUtil::readProcessMemory(m_scanner.getHandle(), (const char*)0xA018350 , 8, block) == 0)
//	{
//		const char* ptr = *(const char**)block.data;
//		log(str_format("Redirect %p\n", ptr));
//		mem_zero(&block, sizeof(block));
//		if (ProcUtil::readProcessMemory(m_scanner.getHandle(), ptr + 400, 8, block) == 0)
//		{
//			const char* ptr = *(const char**)block.data;
//			log(str_format("2nd Redirect %p\n", ptr));
//			mem_zero(&block, sizeof(block));
//			if (ProcUtil::readProcessMemory(m_scanner.getHandle(), ptr + 172, 8, block) == 0)
//			{
//				g_anchor = *(const char**)block.data;
//				log(str_format("G_Anchor = %p\n", g_anchor));
//			}
//		}
//	}
//}

//if ( !g_anchor ) return;

// possible blocks 
// 190820352
// 291975168
// 191905792
// 34316288
// 1052672
// 25579520
// 14741504
// 17973248

