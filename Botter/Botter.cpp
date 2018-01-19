#include "Botter.h"


const float U2M	   = 1.f/60.f; // unit2metesr
const float M2U	   = (1.f/U2M); // unit2metesr

const bool LogToFile = false;

// Minimum bone arrays to be found before starting 
const u32 MiniumNumBoneArrays = 1;

// Aim
const float AimDot = .4f;
const float AimDotV = .2f;
const float MaxDst = 45.f;
const float MinDst = 3.0f;
const float AimSpd = 255;
const float AimZ   = 0.7f * M2U;
const float DistOverDot = 100.f;

// Aim extrapolation
const float AimVelMulplier = 10.f;// 0.01f;
const float AimFollowSoftness = 0.9F;// 0.15f;


// If a bone array moves this fast, consider it in valid array
const float VelMoveThreshold = 0.001f; // m/s
const float VelMoveThresholdHi = 4.5f; // m/s
const float ConsiderMovingSpanTime = 0.4f; // seconds
const u32 NumMovesBeforeValid = 3;
const float VelMaxValid = 10.f; // m/s

const float AllyDistance = 10.f; // meters

const u32 BonePreOffset = 4;


template <typename T> 
T get(const char* p, u32 offset)
{
	return *(T*)(p + offset);
}


// ------------------- Velocity ----------------------------------------------------------------------------------------------------

void Velocity::update(const Vec3& newPos)
{
	float tNow = to_seconds( time_now() );
	float dt = tNow - lastTs;
	if ( dt > 0.1f )
	{
		lastTs = tNow;
		vel = (newPos - oldPos) * (1.f / dt);
		oldPos = newPos;
	}
}


// ------------------- GameCamera ----------------------------------------------------------------------------------------------------


void GameCamera::update()
{
	vel.update(getHeadPos());
}


// ------------------- BoneArray2 ----------------------------------------------------------------------------------------------------


void BoneArray2::update()
{
	if ( missile ) return; // reached impossible velocity 

	float tNow = to_seconds( time_now() );
	vel.update( getPosition() );

	if ( prevBones.empty() ) 
	{
		prevBones.resize(g_numBones);
		for (u32 i = 0; i < g_numBones ; i++)
		{
			prevBones.emplace_back( getBonePosition(i) );
		}
	}
	
	// do in view check
	if ( tNow - lastInViewCheckTs > 0.1f )
	{
		float highest = FLT_MIN;
		inview = false;
		for (u32 i = 0; i < g_numBones ; i++)
		{
			Vec3 bp = getBonePosition(i);
			if ( !(prevBones[i] == bp) )
			{
				inview = true;
				if ( bp.z > highest )
				{
					highest = bp.z;
					lastKnownMovedBoneIdx = i;
				}
				prevBones[i] = bp;
			}
		}
		lastInViewCheckTs = tNow;
	}

	// make bone array valid if has walked for N amount of times at each moment for X seconds
	float velms = vel.velocity().length() * U2M;
	if ( !valid )
	{
		if ( !isMoving )
		{
			if ( velms > VelMoveThreshold && velms < VelMoveThresholdHi )
			{
				isMoving = true;
				startedMovingTs = tNow;
			}
		}
		else 
		{
			if ( velms < VelMoveThreshold || velms > VelMoveThresholdHi )
			{
				// fail
				isMoving = false;
			}
			else if ( tNow - startedMovingTs > ConsiderMovingSpanTime )
			{
				numTimesMoved ++;
				isMoving = false;
				if ( numTimesMoved == NumMovesBeforeValid )
				{
					valid = true;
					Botter::log(str_format( "VALID barray %p %d\n", this->getAddress(), this->getId()));
				}
			}
		}
	}

	// so softening to target
	Vec3 newTarget = getPosition() + vel.velocity() * U2M * AimVelMulplier;
	Vec3 delta = newTarget - targetPos;
	targetPos += delta * AimFollowSoftness;

	// NOTE bones do not update when out of view

	//// if velocity is very low for some time remove bone array
	//if ( velms < 0.000001f )
	//{
	//	if (!lastMoveWasInvalid)
	//	{
	//		lastMoveWasInvalid = true;
	//		lastMoveInvalidTs  = tNow;
	//	}
	//	else if ( tNow - lastMoveInvalidTs > 3.f )
	//	{
	//		Botter::log(str_format("Invalidated barray %p %d (velocity was nearly zero for 3 secs) \n", this->getAddress(), this->getId()));
	//		missile = true;
	//		valid  = false;
	//		inview = false;
	//	}
	//}
	//else
	//{
	//	lastMoveWasInvalid = false;
	//}
}

Vec3 BoneArray2::computeDirTo(const Vec3& pos, float& distOut) const
{
	float dist = getTargetPosition().dist(pos);
	Vec3 dir   = getTargetPosition() - pos;
	distOut = dir.length();
	dir.normalize();
	return dir;
}

Vec3 BoneArray2::getTargetPosition() const
{
	return targetPos;
}

Vec3 BoneArray2::getPosition() const
{
	return getBonePosition( 0 ); // lastKnownMovedBoneIdx );
}

Vec3 BoneArray2::getVelocity() const
{
	return vel.velocity();
}

Null::u32 BoneArray2::getTeam() const
{
	if ( memEntry )
	{
		// bone pre offset is for team 
		u32 team = *(u32*)(memEntry->value.ptr + dynOffset);
		return team;
	}
	return -1;
}

bool BoneArray2::isValid() const
{
	return valid;
}

bool BoneArray2::inView() const
{
	return inview;
}

const char* BoneArray2::getAddress() const
{
	if ( memEntry )
	{
		return memEntry->baseAddress + memEntry->offset + dynOffset;
	}
	return nullptr;
}

Vec3 BoneArray2::getBonePosition(u32 boneIdx) const
{
	if ( memEntry )
	{
		Bone* b = (Bone*)(memEntry->value.ptr + BonePreOffset + dynOffset);
		return Vec3((b + boneIdx)->pos) + Vec3(0, 0, AimZ);
	}
	return Vec3();
}

void BoneArray2::updateDynamicOffset(u32 offset)
{
	dynOffset = offset;
}

// ------------------- Botter ----------------------------------------------------------------------------------------------------

Botter::Botter():
	m_closing(false),
	m_scanner("s2_mp64_ship.exe"),
	m_camScan(nullptr),
	m_boneArrayScan(nullptr),
	m_gameCamScan(nullptr),
	m_gameBoneArrayScan(nullptr),
	m_target(nullptr),
	m_wasAiming(false),
	m_numValidBoneArrays(0),
	m_numInViewBoneArrays(0)
{
	m_numBoneRestartTicks = 0;
	m_uniqueBoneArrayId = 0;
	m_gameCamStateInvalidTs  = 0;
	m_gameBoneStateInvalidTs = 0;
	m_cameraState  = MemoryState::Find;
	m_boneArrayState  = MemoryState::Find;

	file_remove( "Botter_output.txt" );

	m_gameCam.valid = false;
	mem_zero(&m_gameCam, sizeof(m_gameCam));

	if (m_scanner.getHandle())
	{
		m_cameraThread = std::thread([&]() { cameraSearch(); });
		m_boneArrayThread = std::thread([&]() { boneArraySearch(); });
		SetThreadPriority( m_cameraThread.native_handle(), THREAD_MODE_BACKGROUND_BEGIN );
		SetThreadPriority( m_boneArrayThread.native_handle(), THREAD_MODE_BACKGROUND_BEGIN );
		SetThreadAffinityMask( m_cameraThread.native_handle(), 2 );
		SetThreadAffinityMask( m_boneArrayThread.native_handle(), 4 );

	} else  log("Cod ww2 not started..\n");
}

Botter::~Botter()
{
	m_closing = true;
	if (m_boneArrayThread.joinable()) m_boneArrayThread.join();
	if (m_cameraThread.joinable()) m_cameraThread.join();
	if (m_camScan) m_scanner.deleteScan(m_camScan->getId());
	if (m_boneArrayScan) m_scanner.deleteScan(m_boneArrayScan->getId());
}

void Botter::cameraSearch()
{
	while (!m_closing)
	{
		switch (m_cameraState)
		{
		case MemoryState::Find:
			findCamera();
			break;

		case MemoryState::Filter:
			filterCamera();
			break;

		case MemoryState::Wait:
			break;
		}
		sleep_milli(1000);
	}
}

void Botter::boneArraySearch()
{
	while (!m_closing)
	{
		switch (m_boneArrayState)
		{
		case MemoryState::Find:
			findBoneArrays();
			break;

		case MemoryState::Filter:
			filterBoneArrays();
			break;

		case MemoryState::Wait:
			//m_numBoneRestartTicks ++;
			//if ( m_numBoneRestartTicks == 100 )
			//{
			//	m_numBoneRestartTicks = 0;
			//	m_boneArrayState = MemoryState::Find;
			//}
			break;
		}
		sleep_milli(1000);
	}
}

bool Botter::gameTick(u32 tickIdx, float dt)
{
	updateCamera();
	updateBoneArrays();

	if (::GetAsyncKeyState(VK_MENU))
	{
		updateAllies();
	}

	if (::GetAsyncKeyState(VK_F4))
	{
		std::lock_guard<std::mutex> lk(m_boneArrayMutex);
		if ( m_boneArrayState == MemoryState::Wait )
		{
			m_boneArrayState = MemoryState::Find;
		}
	}

	if ( ::GetAsyncKeyState(VK_CAPITAL) /*|| ::GetAsyncKeyState(VK_MENU)*/ ) 
	{
		aim(dt);
		m_wasAiming = true;
	}
	else
	{
		m_wasAiming = false;
		m_target = nullptr;
	}
	return true;
}

void Botter::findCamera()
{
	log("Restaring CAMERA_SEARCH...\n");

	// delete old scan if we had one
	if (m_camScan)
	{
		m_scanner.deleteScan(m_camScan->getId());
		m_camScan = nullptr;
	}

	const u64 startOff = 0x6FFFFFF;
	const u64 endOffs  = 0x9FFFFFF;

	u32 scanIdx = m_scanner.newGenericScan(128, 4, startOff, (endOffs - startOff), ValueType::Memory, false,
										   [&](const char* p, const char* data, u64 offInBlock, u64 blockSize)
	{
		bool bKeepEntry = isValidCamera(data) == 0;
		if (bKeepEntry) log(str_format("Valid camera offset %p block Size %lld\n", p+offInBlock, blockSize));
		return bKeepEntry;
	});

	m_camScan = m_scanner.getScan(scanIdx);
	if (!m_camScan) return;

	u64 num = m_scanner.getNumEntries(m_camScan->getId(),false);
	if (num != 0)
	{
		m_cameraState = MemoryState::Filter;
	}

	log(str_format("Possible camera's after inital search %lld\n", num));
}

void Botter::filterCamera()
{
	if (!m_camScan)
	{
		m_cameraState = MemoryState::Find;
		return;
	}

	log("Filtering camera's..\n");

	m_scanner.filterGenericScan( m_camScan->getId(), 64, 64, [&](const char* newData, const char* oldData, const char* baseAddress, u64 ofsInBlock, u64 blockSize, u32& typeId)
	{
		i32 iRes = isValidCamera(newData);
		if ( iRes != 0 )
		{
			log(str_format("Camera filter failure error: %d\n", iRes));
		}
		return iRes == 0;
	});

	u64 numEntries = m_scanner.getNumEntries(m_camScan->getId(), 0);
	if (numEntries < 40)
	{
		m_scanner.toMemory(m_camScan->getId(), 0);
	}

	log(str_format("Num camera's after filter %lld\n", numEntries));

	// Currently we always find 2 identical camera's (split screen)
	if (numEntries <= 20)
	{
		log("Camera search sufficiently filtered.. Starting update..\n");
		std::lock_guard<std::mutex> lk(m_camMutex);
		m_cameraState  = MemoryState::Wait;
		if (m_gameCamScan) m_scanner.deleteScan(m_gameCamScan->getId());
		m_gameCamScan = m_camScan;
		m_camScan = nullptr;
	}
	
	if (numEntries == 0)
	{
		log("WARNING: No camera's left after filter.\n");
		m_cameraState = MemoryState::Find;
	}
}

void Botter::findBoneArrays()
{
	log("Restarting BONE_ARRAY search..\n");

	// delete old scan if we had one
	if (m_boneArrayScan)
	{
		m_scanner.deleteScan(m_boneArrayScan->getId());
		m_boneArrayScan = nullptr;
	}

	const u64 startAddr = 0x14B000000;
	const u64 endAddr   = 0x15F000000;

	u64 tNow = time_now();

	u32 scanIdx = m_scanner.newGenericScan(32*g_numBones + BonePreOffset, 4, startAddr, endAddr - startAddr, ValueType::Memory, false,
										   [&](const char* p, const char* data, u64 ofsInBlock, u64 blockSize)
	{
		i32 res = isValidBoneArray(data);
		return res == 0;
	});

	m_boneArrayScan = m_scanner.getScan(scanIdx);
	if (!m_boneArrayScan) return;

	u64 num = m_scanner.getNumEntries(m_boneArrayScan->getId(), 0);
	if (num != 0)
	{
		m_boneArrayState = MemoryState::Filter;
	}

	log(str_format("Possible bone arrays after inital search %lld, time %lld ms\n", num, time_now() - tNow));
}

void Botter::filterBoneArrays()
{
	if (!m_boneArrayScan)
	{
		m_boneArrayState = MemoryState::Find;
		return;
	}

	u64 tNow = time_now_nano();

	m_scanner.filterGenericScan( m_boneArrayScan->getId(), 64, 64, [&](const char* newData, const char* oldData, const char* p, u64 ofsInBlock, u64 blockSize, u32& typeId)
	{
		u32 res = isValidBoneArray( newData );
		return res == 0;
	});

	u64 numEntries = m_scanner.getNumEntries(m_boneArrayScan->getId(), 0);
	log(str_format("Num bone arrays after filter %lld, time %lld nano\n", numEntries, time_now_nano() - tNow));

	if (numEntries < 900 && numEntries > 0)
	{
		m_scanner.toMemory(m_boneArrayScan->getId(), 0);
		log("Bone array's sufficiently filtered.. Starting update..\n");
		std::lock_guard<std::mutex> lk(m_boneArrayMutex);
		m_boneArrayState = MemoryState::Wait;
		if (m_gameBoneArrayScan) m_scanner.deleteScan(m_gameBoneArrayScan->getId());
		m_gameBoneArrayScan = m_boneArrayScan;
		m_boneArrayScan = nullptr;
		for (auto& ba : m_boneArrays) delete ba;
		m_boneArrays.clear();
	}
	
	if (numEntries == 0)
	{
		log("WARNING: No bone arrays found after filter..\n");
		m_boneArrayState = MemoryState::Find;
	}
}

void Botter::updateCamera()
{
	m_gameCam.valid = false;

	auto fnCleanupAndResetSearch = [&]()
	{
		std::lock_guard<std::mutex> lk(m_camMutex);
		if (m_gameCamScan)
		{
			m_scanner.deleteScan(m_gameCamScan->getId());
			m_gameCamScan = nullptr;
		}
		assert( m_cameraState == MemoryState::Wait );
		m_cameraState = MemoryState::Find;
	};

	// shared mem acces
	{
		std::lock_guard<std::mutex> lk(m_camMutex);
		if (!m_gameCamScan) return;
	}

	u32 updateIdx = 0;
	//if (m_gameCamScan->numEntries() == 2)
	//	updateIdx = 1;

	// see if can update
	if (!m_scanner.updateEntry(m_gameCamScan->getId(), updateIdx))
	{
		log("WARNING: Could not update camera data..\n");
		fnCleanupAndResetSearch();
		return;
	}

	// validate
	const MemEntry* entry = m_gameCamScan->getEntryAt(updateIdx);
	u32 iRes = isValidCamera(entry->value.ptr);
	if (iRes != 0)
	{
		m_gameCam.valid = false;
		if (0 == m_gameCamStateInvalidTs)
		{
			m_gameCamStateInvalidTs = time_now();
		}
		else if (time_now() - m_gameCamStateInvalidTs > 1000)
		{
			log(str_format("WARNING: Camera no longer valid (Error %d), refinding..\n", iRes));
			fnCleanupAndResetSearch();
		}
		return;
	}

	// Cam is valid, reset invalid ts if was invalid previously
	m_gameCamStateInvalidTs = time_now();

	m_gameCam.entry = entry;
	m_gameCam.update();
	m_gameCam.valid = true;
}


void Botter::updateAllies()
{
	if ( !m_gameCam.valid ) return;

	// shared acces
	std::lock_guard<std::mutex> lk(m_boneArrayMutex);
	if (!m_gameBoneArrayScan) return;

	Vec3 p = m_gameCam.getHeadPos();
	for ( auto ba : m_boneArrays )
	{
		if ( ba->getPosition().dist(p)*U2M < AllyDistance )
		{
			ba->setAlly(true);
		}
	}
}

void Botter::updateBoneArrays()
{
	if ( !m_gameCam.valid ) return;

	// shared acces
	std::lock_guard<std::mutex> lk(m_boneArrayMutex);
	if (!m_gameBoneArrayScan) return;

	// rebuild them
	if ( m_boneArrays.empty() )
	{
		m_gameBoneArrayScan->forEachEntry( [&](MemEntry& entry)
		{
			BoneArray2* ba = new BoneArray2();
			ba->id = m_uniqueBoneArrayId++;
			ba->memEntry = &entry;
			m_boneArrays.emplace_back(ba);
			return true; // continue iterating
		} );
	}

	// update bone array
	u32 numUpdated = m_scanner.updateEntries(m_gameBoneArrayScan->getId(), 16, 16,
											 [&](const char*p, u64 ofsInBlock, const char* newData, const char* oldData, u32& typeId)
	{
		i32 res = isValidBoneArray(newData);
		if ( res != 0 )
		{
			return false; // remove (hint for now), unless some upper or lower offsets do match
		}
		return true; // keep
	},  [&](const MemEntry& onDelEntry)
	{
		// delete bone entry
		for ( auto it=m_boneArrays.begin(); it != m_boneArrays.end(); it++)
		{
			IBoneArray* ba = *it;
			if ( ba->getEntry() == &onDelEntry )
			{
				delete ba;
				m_boneArrays.erase( it );
				break;
			}
		}
	});

	
	for (auto* barray : m_boneArrays)
	{
		barray->update();
	}

	// count num valid boneArrays
	m_numInViewBoneArrays = 0;
	m_numValidBoneArrays  = 0;
	for (auto* barray : m_boneArrays)
	{
		if (barray->isValid())  m_numValidBoneArrays++;
		if (barray->inView())	m_numInViewBoneArrays++;
	}
}

void Botter::aim(float dt)
{
	if (!m_gameCam.valid || m_boneArrays.empty()) 
		return;

	Vec3 viewDir   = m_gameCam.getFacing();
	Vec3 upDir     = m_gameCam.getUpVec();
	Vec3 sideDir   = viewDir.cross(upDir);
	Vec3 ownPos    = m_gameCam.getHeadPos();

	std::lock_guard<std::mutex> lk(m_boneArrayMutex);

	// See if target is still valid
	IBoneArray* curTarget = nullptr;
	if (m_target)
	{
		// try fetch existing target
		for (auto& p : m_boneArrays)
		{
			// refetch if bones are dynamically shifted in memory
			if (  fabs((u64)p->getAddress() - (u64)m_target) <= 64 ) 
			{
				curTarget = p;
				m_target = p->getAddress();
				break;
			}
		}

		// do distance range check
		//if ( curTarget )
		//{
		//	float dist = curTarget->getPosition().dist(ownPos)*U2M;
		//	if (!(dist > MinDst && dist < MaxDst && curTarget->isValid() && curTarget->inView())) 
		//	{
		//		m_target = nullptr;
		//		curTarget = nullptr;
		//	}
		//} else m_target = nullptr;

		// If was aiming but target has become invalid, do not shift to next target, let user press aim again
		if (!m_target && m_wasAiming)
		{
			// do not aim
			return;
		}
	}

	// find new target
	if (!curTarget)
	{
		float closest  = FLT_MAX;
		for (auto& p : m_boneArrays)
		{
			if ( !(p->isValid() && p->inView()) ) continue; // skip
			if ( p->getVelocity().length() < 1.f ) continue;
			if ( p->isAlly() ) continue; 

			float dist;
			Vec3 dir = p->computeDirTo(ownPos, dist);
			dist *= U2M;

			float hDot = dir.dot(viewDir);
			float vDot = fabs(dir.dot(sideDir));

			dist += (1.f - fabs(hDot)) * DistOverDot;
		//	printf("Dist %.3f\n", dist);

			if ((dist > MinDst && dist < MaxDst) && 
				(hDot > AimDot && vDot < AimDotV) &&
				dist < closest)  // find closest in aim range
			{
				curTarget  = p;
				m_target   = p->getAddress();
				closest = dist;
			}
		}
	}

	if (curTarget)
	{
		float dist;
		Vec3 dir = curTarget->computeDirTo(ownPos, dist);
		float kSide = dir.dot(sideDir);
		float kUp   = -dir.dot(upDir);
		INPUT input;
		mem_zero(&input, sizeof(input));
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_MOVE;
		input.mi.dx = (LONG)(kSide*AimSpd);
	//	input.mi.dy = (LONG)(kUp*AimSpd);
		SendInput(1, &input, sizeof(INPUT)); 
		log(str_format("Aiming at target id %d, dist %3fm\n", curTarget->getId(), dist*U2M));
	}
}

void Botter::printCameraStats() const
{
	std::lock_guard<std::mutex> lk(m_camMutex);
	if (!m_gameCam.valid) return;
	Vec3 p = m_gameCam.getPosition();
	Vec3 f = m_gameCam.getFacing();
	Vec3 up = m_gameCam.getUpVec();
	Vec3 head = m_gameCam.getHeadPos();
	Vec3 vel  = m_gameCam.getVelocity();
	log(str_format("Camera Stats:\n"));
	printCameraStats(p, f, up, head, vel);
}

void Botter::printCameraStats(Vec3 p, Vec3 f, Vec3 up, Vec3 head, Vec3 vel) const
{
	log(str_format("\tPos %.3f %.3f %.3f\n", p.x, p.y, p.z));
	log(str_format("\tFac %.3f %.3f %.3f\n", f.x, f.y, f.z));
	log(str_format("\tUp %.3f %.3f %.3f\n", up.x, up.y, up.z));
	log(str_format("\tHead %.3f %.3f %.3f\n", head.x, head.y, head.z));
	log(str_format("\tVel %.3fm/s %.3fkm/h\n", vel.length()*U2M, vel.length()*U2M*3.6f));
}

void Botter::printBoneArrays() const
{
	std::lock_guard<std::mutex> lk(m_camMutex);
	if (!m_gameCam.valid) return;
	std::lock_guard<std::mutex> lk2(m_boneArrayMutex);
	const char* prevAddr = nullptr;
	u32 totalEntries = 0;
	bool bFoundSpecific = false;
	for (const IBoneArray* p : m_boneArrays)
	{
		printBoneArray( p );
		totalEntries += p->numBones();
	}

	for (const IBoneArray* p : m_boneArrays)
	{
		if ( p->getId() == 0 )
		{
			log("\nPossible SELF:: \n");
			printBoneArray(p);
		}
	}

	log(str_format("\n\nTotal entries: %d (Found specific %s)\n", totalEntries, (bFoundSpecific?"Yes":"No")));
}

void Botter::printBoneArraysSimple() const
{
	std::lock_guard<std::mutex> lk(m_camMutex);
	if (!m_gameCam.valid) return;
	std::lock_guard<std::mutex> lk2(m_boneArrayMutex);
	u32 idx = 0;
	for (const IBoneArray* p : m_boneArrays)
	{
	//	if ( p->isValid() )
		{
			Vec3 pos = p->getBonePosition(0);
			Vec3 vel = p->getVelocity();
			u32 team = p->getTeam();
			float dist = pos.dist(m_gameCam.getHeadPos());

			log(str_format("\tPlayer: %d | %d | %p | team %xd | in view %s | %.3f %.3f %.3f | %.3fms | Dst %.3f\n",
							idx, p->getId(), p->getAddress(), p->getTeam(), ( p->inView()?"yes":"no"),
							pos.x, pos.y, pos.z, vel.length()*U2M, dist*U2M));
			idx++;
		}
	}
}

void Botter::printBoneArray(const IBoneArray* ba) const
{
	log(str_format("\n---- BoneArray %p id %d Valid %s -----\n", ba->getAddress(), ba->getId(), (ba->isValid()?"Yes":"No")));
	for (u32 i = 0; i < ba->numBones() ; i++)
	{
		Vec3 pos   = ba->getBonePosition(i);
		float dist = pos.dist(m_gameCam.getHeadPos()) * U2M;
		log(str_format("Bone pos %.3f %.3f %.3f | Dist: %.3f \n", pos.x, pos.y, pos.z, dist));
	}
}

void Botter::printBoneArrayRaw(const char* data) const
{
	Bone* b = (Bone*)data;
	for (u32 i = 0; i < g_numBones ; i++)
	{
		log(str_format("Bone Pos: %f %f %f %f | Rot %f %f %f %f\n", b->pos.x, b->pos.y, b->pos.z, b->pos.w, b->rot.x, b->rot.y, b->rot.z, b->rot.w));
		b++;
	}
}

i32 Botter::isValidCamera(const char* p) const
{
	Vec3 position = *(Vec3*)(p);
	Vec3 facing   = *(Vec3*)(p+16);
	Vec3 headPos  = *(Vec3*)(p+32);
	//Vec3 upVec	  = *(Vec3*)(p+44);
	Vec3 upVec	  = *(Vec3*)(p+68);
	float alw4	  = *(float*)(p+104);

	i32 res = validPosition(position*4.f); // to scale to normal position
	if (res != 0)
		return res;
		
	res = validPosition(headPos);
	if (res != 0)
		return res;

	if (!validVector(facing))
		return -4;

	if (!validVector(upVec))
		return -5;

	if (!(alw4 >= 3.99f && alw4 <= 4.01f))
		return -6;

	return 0;
}

u32 Botter::isValidPlayerBot(const char* data) const
{
	// this directly finds a bot in case playing against bots

	float* fl = (float*) data;
	if ( *(fl + 0) > 2.f ) return -1;
	if ( *(fl + 1) > 2.f ) return -2;
	if ( *(fl + 2) > 2.f ) return -3;

	u32 kVal0 = *(u32*) (data + 12);
	u32 kVal1 = *(u32*) (data + 16);
	if (kVal0 != 0x64) return -4;
	if (!(kVal1 == 0x1 || kVal1 == 0x0))  return -5;

	u32 kVal3 = *(u32*) (data + 20);
	u32 kVal4 = *(u32*) (data + 24);

	if (kVal3 != 0x4D800000)  return -6;
	if (kVal4 != 0x4D800000 ) return -7;

	Vec3 pos =  *(Vec3*) (data + 0x5C);
	if (!validPosition(pos) && !nearZero(pos)) return -8;
	Vec3 vec =  *(Vec3*) (data + 0x5C + 12);
	if (!validVector(vec) && !nearZero(vec)) return -9;

	u32 kVal5 = *(u32*) (data + 0x88);
	if ( kVal5 != 0x48800000 ) return -10;

	return 0;
}

Null::u32 Botter::isValidBoneArray(const char* data) const
{
	u32 iAnchor = *(u32*)(data + 0);
	if (!(iAnchor == 0x3F800000)) return -77;// ||  iAnchor == 0)) return -77;

	Bone* b = (Bone*)(data + BonePreOffset);
	for (u32 i = 0; i < g_numBones; i++, b++)
	{
		i32 res = validPosition(b->pos);
		if (res != 0) return res;
		res = validQuat(b->rot);
		if (res != 0) return res;
		//if ( fabs(b->pos.w) > 0.01f ) return -95; // THIS HAPPENS!!
	}

	//// check distances of bones
	//b = (Bone*)(data + BonePreOffset);
	//Vec3 anchor = b->pos;
	//b++; // go next bone
	//for (u32 i = 1; i < g_numBones ; i++, b++)
	//{
	//	Vec3 pos = b->pos;
	//	float dist = pos.dist(anchor)*U2M;
	//	if ( dist > 5.f ) return -250;
	////	if ( dist < 0.001f ) return -251;
	//}

	return 0;
}

bool Botter::nearZero(float f) const
{
	return f >= -0.01f && f <= 0.01f;
	//return (f > -20.101f && f < 20.101f);
}

bool Botter::nearZero(Vec3 f) const
{
	return f.length() < 20.f;
	//return nearZero(f.x) && nearZero(f.y) && nearZero(f.z);
}

bool Botter::isSmall(float f) const
{
	return f > -50 && f < 50;
}

bool Botter::isSmall(Vec3 f) const
{
//	return isSmall(f.x) || isSmall(f.y) || isSmall(f.z);
	u32 t=0;
	for (float i : f.m)
	{
		if ( isSmall(i) ) t++;
	}
	if ( t >= 2 ) return true;

	return isSmall(f.z);

	//return (isSmall(f.x) && !isSmall(f.y) && isSmall(f.z)) || 
	//	   (!isSmall(f.x) && isSmall(f.y) && isSmall(f.z));	
}

bool Botter::isSimilar(Vec3 f) const
{
	float dx = fabs( f.x-f.y );
	float dy = fabs( f.x-f.z );
	float dz = fabs( f.y-f.z );
	u32 t = 0;
	if ( dx < 0.01f ) t++;
	if ( dx < 0.01f ) t++;
	if ( dz < 0.01f ) t++;
	return t >= 1;
}

bool Botter::zOrNorm(float f) const
{
	return isnormal(f) || f==0.f;
}

bool Botter::isIntegral(float f) const
{
	//return false;
//	return ((float)i32(f)) == f;
	//// only allow 3 decimals
	i64 iTemp = (i64)(f * 1000.f);
	float kd = (float)iTemp;
	float fd = float(kd * 0.001);
	return float(i32(fd)) == fd;
}

bool Botter::isValidInt(i32 i) const
{
	return i >= -1000 && i <= 1000;
}

u32 Botter::insideWorld(float f) const
{
	if ( f < -10000.f ) return -50;
	if ( f > 10000.f ) return -51;
//	if ( isIntegral(f) ) return -52;	INTEGRAL HAPPENS ON RESPAWNS!
	// if ( !zOrNorm(f) ) return -53;
	if ( !isnormal(f) ) return -54;
	return 0;
}

bool Botter::validUp(float f) const
{
	return f >= -1500 && f < 3000 && isnormal(f); // zOrNorm(f);
	//return (f >= -50.f && f <= 1300.f) /* !isIntegral(f)*/ && zOrNorm(f);
}

u32 Botter::validPosition(const Vec3& pos) const
{
	if ( nearZero(pos) ) return -6;
	u32 res = insideWorld(pos.x);
	if (res != 0) return res;
	res = insideWorld(pos.y);
	if (res != 0) return res;
	if ( !validUp(pos.z) ) return -3;	 
//	if ( isSmall(pos) ) return -4;			// THIS HAPPENS FOR THE POSITION OF CAMERA
//	if ( isSimilar(pos) ) return -5;		// THIS ACTUALLY HAPPENS ON RESPAWNS..
	return 0;
}

void Botter::log(const String& msg)
{
	printf("%s", msg.c_str());
	if ( LogToFile )
	{
		File f = file_open("Botter_log.txt", "a");
		if (f)
		{
			file_print(f, msg);
			file_close(f);
		}
	}
}

u32 Botter::validQuat(const Vec4& q) const
{
	float l = q.length();
	if ( !(l >= 0.85f && l <= 1.15f) ) return -100;
	if ( !isnormal(l) ) return -101;
	if ( !(zOrNorm(q.x) && zOrNorm(q.y) && zOrNorm(q.z) && zOrNorm(q.w)) ) return -102;
	return 0;
}

bool Botter::validVector(const Vec3& v) const
{
	float l = v.length();
	return l >= 0.95f && l <= 1.05f && isnormal(l) && 
			zOrNorm(v.x) && zOrNorm(v.y) && zOrNorm(v.z);
}

