#include "Botter.h"

#include "../3rdParty/inih-master/cpp/IniReader.h"

float U2M	   = 1.f/60.f; // unit2metesr
float M2U	   = (1.f/U2M); // unit2metesr

bool PrintBoneArrays = true;
bool PrintOnlyValid = true;
bool LogToFile = false;
bool AlwaysAim = false;
bool CheckAlly = true;
bool AimVertical = false;
bool AllowSwitching = false;
bool AllowReject = false;
bool CanLooseTarget = false;
float AllyDistance = 10.f; // meters
float GroupDist = 1.2f;
float LooseTargetDist=10; // meters
u32 CamIndex = 0;


// Aim
float AimDot = .4f;
float AimDotV = .2f;
float MaxDst = 45.f;
float MinDst = 3.0f;
float AimSpd = 255;
float AimSpdV = 55;
float AimZ   = 0.7f * M2U;
float DistDotScale = 10.f;
float DistConstant = 1.f;
float DistOverDotExp = 1.f;
float AimPushBackLength = -20.f;
float MinRetargetTime = .2f;
float FocusMultiplier = 2.f;
float VelocityFailThreshold = 1.0f;
float AimVelMulplier = 10.f;// 0.01f;
float AimFollowSoftness = 0.9F;// 0.15f;


// If a bone array moves this fast, consider it in valid array
float VelMoveThreshold = 0.01f; // m/s
float VelMoveThresholdHi = 6.5f; // m/s
float ConsiderMovingSpanTime = 0.4f; // seconds
float VelMaxValid = 10.f; // m/s
float MinMoveDist = 5.f; // meters
u32 NumMovesBeforeValid = 3;
u32 FilterBoneArrayThreshold = 500;
u64 BoneArrayStartSearch = 0x150000000;
u64 BoneArrayEndSearch   = 0x155000000;

// invalidate
float RemoveBoneArrayIfNoProgressFor = 120.f;


const u32 BonePreOffset = 4;


template <typename T> 
T get(const char* p, u32 offset)
{
	return *(T*)(p + offset);
}


// ------------------- Velocity ----------------------------------------------------------------------------------------------------

void Velocity::update(const Vec3& newPos, float tNow)
{
	float dt = tNow - lastTs;
	if ( dt > 0.1f )
	{
		lastTs = tNow;
		vel = (newPos - oldPos) * (1.f / dt);
		oldPos = newPos;
	}
}


// ------------------- GameCamera ----------------------------------------------------------------------------------------------------


void GameCamera::update(float dt, float tNow)
{
	vel.update(getHeadPos(), tNow);
}


// ------------------- BoneArray2 ----------------------------------------------------------------------------------------------------


void BoneArray2::update(float dt, float tNow)
{
	if ( _isDead ) return; // reached impossible velocity 

	if ( dt < 0 ) dt = 0.0001f;
	if ( dt > 1.f ) dt = 1.f;

	setDirty(true);
	vel.update( getPosition(), tNow );

	if ( prevBones.empty() ) 
	{
		prevBones.resize(g_numBones);
		for (u32 i = 0; i < g_numBones ; i++)
		{
			prevBones.emplace_back( getBonePosition(i) );
		}
	}

	bool validMove = didValidMove();

	updateInView(tNow);
	updateValidation(tNow, dt, validMove);
	updateInvalidate(tNow, validMove);
	updateTargetPoint();
}

void BoneArray2::updateInView(float tNow)
{
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
}

void BoneArray2::updateValidation(float tNow, float dt, bool validMove)
{
	float velms = vel.velocity().length() * U2M;

	// keep updating dist moved even when already valid
	if ( validMove ) distTravalled += velms * dt;

	if ( valid ) return;

	if ( !isMoving )
	{
		if ( validMove )
		{
			isMoving = true;
			startedMovingTs = tNow;
		}
	}
	else 
	{
		if ( !validMove )
		{
			// fail
			isMoving = false;
		}
		else if ( tNow - startedMovingTs > ConsiderMovingSpanTime )
		{
			numTimesMoved ++;
			if ( numTimesMoved >= NumMovesBeforeValid && distTravalled > MinMoveDist )
			{
				isMoving = false;
				valid = true;
		//		Botter::log(str_format( "VALID barray %p %d\n", this->getAddress(), this->getId()));
			}
		}
	}
}

void BoneArray2::updateInvalidate(float tNow, bool validMove)
{
	// NOTE bones do not update when out of view
	if ( RemoveBoneArrayIfNoProgressFor < 0 ) return;

	if (validMove) lastValidMoveTs = tNow;

	if ( lastValidMoveTs < 0.001f ) lastValidMoveTs = tNow;

	if ( tNow - lastValidMoveTs > RemoveBoneArrayIfNoProgressFor )
	{
	//	Botter::log(str_format("Invalidated barray %p %d \n", this->getAddress(), this->getId()));
		_isDead = true;
		valid  = false;
		inview = false;
	}
}

void BoneArray2::updateTargetPoint()
{
	Vec3 lookDir; 
	if ( memEntry->typeId != 0 )
		lookDir = Vec3(1,0,0);
	else
		lookDir = (getBoneRotation(0) * Vec3(0.f, 1, 0));
	Vec3 vel2D   = vel.velocity();
	vel2D.z = 0.f;
	Vec3 newTarget = (getPosition() + Vec3(0, 0, AimZ*M2U) + lookDir*AimPushBackLength*M2U) + (vel2D * U2M * AimVelMulplier);
	Vec3 delta = newTarget - targetPos;
	targetPos += delta * AimFollowSoftness;
}

bool BoneArray2::didValidMove() const
{
	if (0 != Botter::validPosition(getPosition())) return false;
	float velms = vel.velocity().length() * U2M;
	return (velms > VelMoveThreshold && velms < VelMoveThresholdHi);// && fabs(getAngle()-90) > 0.01f;
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

float BoneArray2::getAngle() const
{
	Vec4 quat = getBoneRotation( 0 );
	Vec3 look = quat * Vec3(1, 0, 0);
	look.normalize();
	float ang = atan2( look.x, look.y );
	return ang/3.14159f * 180.f;
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
	if (!dirty) return cachePos;

	if ( memEntry )
	{
		if ( memEntry->typeId == 0 )
		{
			Bone* b = (Bone*)(memEntry->value.ptr + BonePreOffset + dynOffset);
			cachePos = Vec3((b + boneIdx)->pos);
		} 
		else
		{
			cachePos = *(Vec3*)(memEntry->value.ptr + 0x3C + 0 + dynOffset);
		}
	}
	
	dirty=false;
	return cachePos;
}

Vec4 BoneArray2::getBoneRotation(u32 boneIdx) const
{
	if ( memEntry && memEntry->typeId == 0 )
	{
		Bone* b = (Bone*)(memEntry->value.ptr + BonePreOffset + dynOffset);
		return (b + boneIdx)->rot;
	}
	return Vec4(0, 0, 0, 1);
}

void BoneArray2::updateDynamicOffset(u32 offset)
{
	dynOffset = offset;
}

Null::u32 BoneArray2::getTypeId() const
{
	if ( memEntry ) return memEntry->typeId;
	return 0;
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
	m_numValidBoneArrays(0),
	m_numInViewBoneArrays(0)
{
	m_numBoneRestartTicks = 0;
	m_uniqueBoneArrayId = 0;
	m_gameCamStateInvalidTs  = 0;
	m_gameBoneStateInvalidTs = 0;
	m_lastTargetResetTs = 0;
	m_cameraState  = MemoryState::Find;
	m_boneArrayState  = MemoryState::Find;
	m_isFocussing = false;

	file_remove( "Botter_output.txt" );

	m_gameCam.valid = false;
	mem_zero(&m_gameCam, sizeof(m_gameCam));

	if (m_scanner.getHandle())
	{
		log("------ Found COD WW2. Starting bot... ------\n\n");

		reloadConfig();

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

bool Botter::gameTick(u32 tickIdx, float dt, float tNow)
{
	updateCamera(dt, tNow);
	updateBoneArrays(dt, tNow);

	if (::GetAsyncKeyState(VK_MENU) & 1) // PRESSED
	{
		if ( m_target != nullptr )
		{
			allyTarget();
		}
		else
		{
			updateAllies();
		}
	}

	if (::GetAsyncKeyState(VK_F2))
	{
		reloadConfig();
	}

	// happens auto
	//if (::GetAsyncKeyState(VK_F3) & 1)
	//{
	//	std::lock_guard<std::mutex> lk(m_camMutex);
	//	if ( m_cameraState == MemoryState::Wait )
	//	{
	//		if (m_gameCamScan)
	//		{
	//			m_scanner.deleteScan(m_gameCamScan->getId());
	//			m_gameCamScan = nullptr;
	//			m_gameCam.valid = false;
	//		}
	//		m_cameraState = MemoryState::Find;
	//	}
	//}

	if (::GetAsyncKeyState(VK_F4))
	{
		std::lock_guard<std::mutex> lk(m_boneArrayMutex);
		if ( m_boneArrayState == MemoryState::Wait )
		{
			m_boneArrayState = MemoryState::Find;
		}
	}

	if ((::GetAsyncKeyState(VK_F5) & 1) != 0)  { AimSpd /= 1.25f; printf("Aim speed now %.3f\n", AimSpd); }
	if ((::GetAsyncKeyState(VK_F6) & 1) != 0)  { AimSpd *= 1.25f; printf("Aim speed now %.3f\n", AimSpd); }

	m_isFocussing = false;
	if (::GetAsyncKeyState(VK_RBUTTON)) m_isFocussing = true;

	
	if ( AlwaysAim || ::GetAsyncKeyState(VK_CAPITAL) )
	{
		float tNow = to_seconds(time_now());
		if ( tNow - m_lastTargetResetTs > MinRetargetTime )
		{
			aim(tNow, dt);
		}
	}
	else
	{
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
		//	log(str_format("Camera filter failure error: %d\n", iRes));
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

	u64 startAddr = BoneArrayStartSearch;
	u64 endAddr   = BoneArrayEndSearch;

	u64 tNow = time_now();

	u32 scanIdx = m_scanner.newGenericScan(0x3c+64/* 32*g_numBones + BonePreOffset*/, 4, startAddr, endAddr - startAddr, ValueType::Memory, false,
										   [&](const char* p, const char* data, u64 ofsInBlock, u64 blockSize)
	{
		u32 type;
		i32 res = isValidBoneArray(data, type);
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
		u32 res = isValidBoneArray( newData, typeId );
		return res == 0;
	});

	u64 numEntries = m_scanner.getNumEntries(m_boneArrayScan->getId(), 0);
	log(str_format("Num bone arrays after filter %lld, time %lld nano\n", numEntries, time_now_nano() - tNow));

	if (numEntries < FilterBoneArrayThreshold && numEntries > 0)
	{
		m_scanner.toMemory(m_boneArrayScan->getId(), 0);
		log("Bone array's sufficiently filtered.. Starting update..\n");
		std::lock_guard<std::mutex> lk(m_boneArrayMutex);
		m_boneArrayState = MemoryState::Wait;
		m_boneArrayRefreshed = true;
	}
	
	if (numEntries == 0)
	{
		log("WARNING: No bone arrays found after filter..\n");
		m_boneArrayState = MemoryState::Find;
	}
}

void Botter::updateCamera(float dt, float tNow)
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
	if (CamIndex < m_gameCamScan->numEntries())
		updateIdx = CamIndex;

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
			m_gameCamStateInvalidTs = tNow;
		}
		else if (tNow - m_gameCamStateInvalidTs > 1000)
		{
			log(str_format("WARNING: Camera no longer valid (Error %d), refinding..\n", iRes));
			fnCleanupAndResetSearch();
		}
		return;
	}

	// Cam is valid, reset invalid ts if was invalid previously
	m_gameCamStateInvalidTs = tNow;

	m_gameCam.entry = entry;
	m_gameCam.update(dt, tNow);
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

void Botter::allyTarget()
{
	IBoneArray* curTarget = fetchCurrentTarget();
	if ( curTarget != nullptr )
	{
		forAllBoneArraysInRange( GroupDist*M2U, curTarget, [&](IBoneArray* ba)
		{
			if (ba->isValid() && ba->inView())
			{
				ba->setAlly(true);
			}
		});
		curTarget->setAlly( true );
		resetTarget();
	}
}

void Botter::reloadConfig()
{
	INIReader reader("config.ini");

	if (reader.ParseError() < 0) 
	{
		log("Failed to reload config.ini\n");
	}
	else
	{ // Read settings from ini

		PrintBoneArrays = reader.GetBoolean("general", "printBoneArrays", true);
		PrintOnlyValid = reader.GetBoolean("general", "printOnlyValid", true);
		LogToFile = reader.GetBoolean("general", "logToFile", false);
		AlwaysAim = reader.GetBoolean("general", "alwaysAim", false);
		CheckAlly = reader.GetBoolean("general", "checkAlly", true);
		AimVertical = reader.GetBoolean("general", "aimVertical", true);
		AllowSwitching = reader.GetBoolean("general", "allowSwitching", true);
		AllowReject  = reader.GetBoolean("general", "allowReject", true);
		CanLooseTarget = reader.GetBoolean("general", "canLooseTarget", false);
		AllyDistance = (float)reader.GetReal("general", "allyDistance", 8.5); // meters
		GroupDist = (float)reader.GetReal("general", "groupDist", 1.2f);
		DesiredTickRate = reader.GetInteger("general", "desiredTickRate", 80); // meters
		LooseTargetDist = (float)reader.GetReal("general", "looseTargetDist", 10); // metrs
		CamIndex = reader.GetInteger("general", "camIndex", 0);
		
		 // Aim
		AimDot  = (float) reader.GetReal("aim", "aimDot", .4);
		AimDotV = (float)reader.GetReal("aim", "aimDotV", .2);
		MaxDst  = (float)reader.GetReal("aim", "maxDst", 45);
		MinDst  = (float)reader.GetReal("aim", "minDst", 3);
		AimSpd  = (float)reader.GetReal("aim", "aimSpd", 255);
		AimSpdV = (float)reader.GetReal("aim", "aimSpdV", 55);
		AimZ    = (float)reader.GetReal("aim", "aimZ", .7) * M2U;
		AimPushBackLength = (float)reader.GetReal("aim", "aimPushBackLength", .3) * M2U;
		DistDotScale = (float)reader.GetReal("aim", "distDotScale", 10.f);
		DistConstant = (float)reader.GetReal("aim", "distConstant", 1.f);
		DistOverDotExp = (float)reader.GetReal("aim", "distOverDotExp", 2.f);
		AimVelMulplier = (float)reader.GetReal("aim", "aimVelMultiplier", 10);
		AimFollowSoftness = (float)reader.GetReal("aim", "aimFollowSoftness", .9);
		MinRetargetTime = (float)reader.GetReal("aim", "minRetargetTime", .2);
		FocusMultiplier = (float)reader.GetReal("aim", "focusMultiplier", 2.5f);
		VelocityFailThreshold = (float)reader.GetReal("aim", "velocityFailThreshold", 1.0f);

		// If a bone array moves this fast, consider it in valid array
		VelMoveThreshold = (float)reader.GetReal("validation", "velMoveThreshold", .001); // m/s
		VelMoveThresholdHi =(float) reader.GetReal("validation", "velMoveThresholdHi", 4.5); // m/s
		ConsiderMovingSpanTime = (float)reader.GetReal("validation", "considerMovingSpanTime", 0.4); // seconds
		NumMovesBeforeValid = reader.GetInteger("validation", "numMovesBeforeValid", 5);
		MinMoveDist = (float) reader.GetReal("validation", "minMoveDist", 5);
		FilterBoneArrayThreshold =  reader.GetInteger("validation", "filterBoneArrayThreshold", 1500);
		BoneArrayStartSearch =  reader.GetInteger64("validation", "boneArrayStartSearch", 0x150000000);
		BoneArrayEndSearch =  reader.GetInteger64("validation", "boneArrayEndSearch", 0x155000000);

		RemoveBoneArrayIfNoProgressFor = (float) reader.GetReal("invalidation", "removeIfNoProgressFor", 120.f);


		log("Reloaded config.ini\n");
	}
}

void Botter::updateBoneArrays(float dt, float tNow)
{
	if ( !m_gameCam.valid ) return;

	// shared acces
	{
		std::lock_guard<std::mutex> lk(m_boneArrayMutex);
		if (m_boneArrayRefreshed && m_boneArrayState == MemoryState::Wait)
		{
			if (m_gameBoneArrayScan)
			{
				// delete old
				m_scanner.deleteScan(m_gameBoneArrayScan->getId());
				m_gameBoneArrayScan = nullptr;
			}

			m_gameBoneArrayScan = m_boneArrayScan;
			m_boneArrayScan = nullptr;
		}
	}

	if (!m_gameBoneArrayScan) return;

	if ( m_boneArrayRefreshed )
	{
		// delete old
		for (auto& ba : m_boneArrays) delete ba;
		m_boneArrays.clear();

		// build new
		m_gameBoneArrayScan->forEachEntry( [&](MemEntry& entry)
		{
			BoneArray2* ba = new BoneArray2();
			ba->id = m_uniqueBoneArrayId++;
			ba->memEntry = &entry;
			m_boneArrays.emplace_back(ba);
			return true; // continue iterating
		} );

		m_boneArrayRefreshed = false;
	}

	// update bone array
	u32 numUpdated = m_scanner.updateEntries(m_gameBoneArrayScan->getId(), 16, 16,
											 [&](const char*p, u64 ofsInBlock, const char* newData, const char* oldData, u32& typeId)
	{
		i32 res = isValidBoneArray(newData, typeId);
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
		barray->update(dt, tNow);
	}

	// remove dead bone arrays
	u32 numBoneArraysDeleted = 0;
	for (auto it=m_boneArrays.begin(); it!=m_boneArrays.end();)
	{
		IBoneArray* ba = *it;
		if (ba->isDead()) 
		{
			delete ba;
			it = m_boneArrays.erase(it);
			numBoneArraysDeleted++;
		} else it++;
	}
	if (numBoneArraysDeleted>0) 
	{
		log(str_format("Deleted %d bone arrays because became inoperatable\n", numBoneArraysDeleted));
	}

	// count num valid boneArrays
	m_numInViewBoneArrays = 0;
	m_numValidBoneArrays  = 0;
	for (auto* barray : m_boneArrays)
	{
		if (barray->isValid())  m_numValidBoneArrays++;
		if (barray->inView())	m_numInViewBoneArrays++;
	}

	// sort on dist travalled
	//std::sort( m_boneArrays .begin(), m_boneArrays.end(), [&](auto l, auto r)
	//{
	//	return l->distMoved() > r->distMoved();
	//});
	
}

u32 Botter::isTargetValid(IBoneArray* p) const
{
	if ( !p->isValid() ) return -1; 
	if ( !p->inView() ) return -4;
	if ( p->getVelocity().length() < VelocityFailThreshold ) return -2;
	if ( CheckAlly && p->isAlly() ) return -3;
	return 0;
}

IBoneArray* Botter::findBestTarget() const
{
	if (!m_gameCam.valid) 
	{
		return nullptr;
	}

	Vec3 viewDir   = m_gameCam.getFacing();
	Vec3 upDir     = m_gameCam.getUpVec();
	Vec3 ownPos    = m_gameCam.getHeadPos();

	float closest = FLT_MAX;
	float eqDist  = FLT_MAX;
	IBoneArray* best = nullptr;
	//printf("begin\n");
	for (auto& p : m_boneArrays)
	{
		u32 res = isTargetValid(p);
		if ( res != 0 )
		{
		//	printf("Target id %d lock on fail, reason %d\n", p->getId(), res);
			continue;
		}

		float dist;
		Vec3 dir = (p->getPosition()-ownPos);
		dist = dir.length();
		dir  *= (1.f/dist);
		dist *= U2M;
		
		float vDot = fabs(dir.dot(upDir));
		dir.z = 0;
		viewDir.z = 0;
		dir.normalize();
		viewDir.normalize();
		float hDot = dir.dot(viewDir);

		// functional dist 
		float alpha = 1-hDot;
		alpha = max(0.f, alpha);
		alpha = min(1.f, alpha);
		float fDist = pow(alpha*DistDotScale, DistOverDotExp) * dist + dist*DistConstant;
		//float fDist = (1-alpha)*dist + alpha*hDot*DotBaseConstant;

		if ((dist > MinDst && dist < MaxDst) && 
			(hDot > AimDot && vDot < AimDotV) &&
			(fDist < closest))
		{
	//		printf("Setting ID %d was %d\n", p->getId(), best?best->getId():-1);
			best = p;
			closest = fDist;
			eqDist  = dist;
		}
		//else
		//{
		//	printf("Skipping because: dist = %.3f, hDot = %.3f vDot = %.3f fdist = %.3f, closest %.3f, %d %d %d %d %d\n", dist, hDot, vDot, fDist, closest,
		//		   (dist > MinDst), (dist < MaxDst), (hDot > AimDot), (vDot < AimDotV), (fDist < closest));
		//}
	}
	return best;
}

void Botter::resetTarget()
{
	float tNow = to_seconds(time_now());
	m_lastTargetResetTs = tNow;
	m_target = nullptr;
}

void Botter::aim(float tNow, float dt)
{
	if (!m_gameCam.valid || m_boneArrays.empty()) 
	{
	//	printf("Not aiming, cam not valid or bone arrays empty!\n");
		return;
	}

	Vec3 viewDir   = m_gameCam.getFacing();
	Vec3 upDir     = m_gameCam.getUpVec();
	Vec3 sideDir   = viewDir.cross(upDir);
	Vec3 ownPos    = m_gameCam.getHeadPos();

	// See if target is still valid
	IBoneArray* curTarget = nullptr;
	if (m_target)
	{
		// try fetch existing target
		curTarget = fetchCurrentTarget();

		if ( curTarget )
		{
			// update address as it may have been shifted slightly in memory
			m_target = curTarget->getAddress();

			// loose target on dist exceeded
			float dist = curTarget->getPosition().dist(m_lastTargetPos) * U2M;
			if ( dist > LooseTargetDist ) 
			{
				resetTarget();
				Botter::log(str_format("Lost target (LooseTargetDist exceeded %.3fM)\n", dist));
			}

			// loose target on velocity fail
			float curVel = curTarget->getVelocity().length();
			if ( CanLooseTarget && (curVel < VelocityFailThreshold) )
			{
				resetTarget();
				Botter::log(str_format("Lost target (Velocity too low %.3fms)\n", curVel));
			}
		}
		else 
		{
			Botter::log("Fetch FAILED\n");
			resetTarget();
		}

		// if still has target, see if there is better one if switching targets is allowed
		if ( m_target && AllowSwitching && (tNow - m_lastTargetResetTs < MinRetargetTime) )
		{
			IBoneArray* curBest = findBestTarget();
			if ( curBest != curTarget && curBest != nullptr )
			{
				Botter::log("Switching target\n");
				m_target = nullptr; // do not reset time again
			}
		}

		// Auto switch to new target only if some time elapsed
		if (!m_target && (tNow - m_lastTargetResetTs < MinRetargetTime) )
		{
			// do not aim
			Botter::log("Not aiming, not allowed yet..\n");
			return;
		}
	}

	// find new target
	if (!curTarget)
	{
	//	printf("Refinding target..\n");
		curTarget = findBestTarget();
		if ( curTarget )
		{
			Botter::log(str_format("Found new target: ID %d\n", curTarget->getId()));
			m_target = curTarget->getAddress();
		}
	}

	if (curTarget)
	{
		m_lastTargetPos = curTarget->getTargetPosition();// avgTargetPos;

		// skip aim if we sent mouse input but our view dir did not get adjusted.
		// this is to prevent a bug that we keep sending mouse input even if the view dir does not change
		// and so the players spins round
		if ( AllowReject && m_lastViewDir == viewDir ) 
		{
		//	printf("Rejecting..\n");
			return;
		}
		m_lastViewDir = viewDir;

		// cur aim speed
		float aimSpeed  = AimSpd * (m_isFocussing ? FocusMultiplier : 1.f);
		float aimSpeedV = AimSpdV * (m_isFocussing ? FocusMultiplier : 1.f);
	//	printf("Aimspeed %.3f \n", aimSpeed);

		Vec3 dir  = (m_lastTargetPos - ownPos);
		dir.normalize();

		float kSide = dir.dot(sideDir);
		float kUp   = -dir.dot(upDir);
		INPUT input;
		mem_zero(&input, sizeof(input));
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_MOVE;
		input.mi.dx = (LONG)(kSide*aimSpeed);
		if ( AimVertical )
		{
			input.mi.dy = (LONG)(kUp*aimSpeedV);
		}
		SendInput(1, &input, sizeof(INPUT)); 
	//	log(str_format("Aiming at target id %d, dist %3fm\n", curTarget->getId(), dist*U2M));
	}
}

IBoneArray* Botter::fetchCurrentTarget() const
{
	for (auto& p : m_boneArrays)
	{
		// refetch if bones are dynamically shifted in memory
		if ( fabs((u64)p->getAddress() - (u64)m_target) <= 64 ) 
		{
			return p;
		}
	}
	return nullptr;
}

void Botter::forAllBoneArraysInRange(float range, const IBoneArray* from, const std::function<void( IBoneArray*)>& cb)
{
	if (!m_gameCam.valid) return;
	Vec3 camPos = m_gameCam.getPosition();
	float danchor = from->getPosition().dist(camPos);
	for ( auto& ba : m_boneArrays )
	{
		float d = ba->getPosition().dist(camPos);
		if ( fabs(d - danchor) <= range )
		{
			cb( ba );
		}
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
	if (!PrintBoneArrays) return;
	std::lock_guard<std::mutex> lk(m_camMutex);
	if (!m_gameCam.valid) return;
	u32 idx = 0;
	for (const IBoneArray* p : m_boneArrays)
	{
		if ( !PrintOnlyValid || p->isValid() )
		{
			Vec3 pos = p->getTargetPosition();
			Vec3 vel = p->getVelocity();
			u32 team = p->getTeam();
			float ang = p->getAngle();
			float dist = pos.dist(m_gameCam.getHeadPos());
			float dMoved = p->distMoved();
			u32 typeId = p->getTypeId();
			log(str_format("%p | %d | %d | tid %d | valid %s | visb %s | %.3f %.3f %.3f | %.3fms | Dst %.3f | Ang %.3f | Moved %.3fM\n",
							p->getAddress(), idx, p->getId(), typeId, (p->isValid()?"yes":"no"), (p->inView()?"yes":"no"),
							pos.x, pos.y, pos.z, vel.length()*U2M, dist*U2M, ang, dMoved));
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

Null::u32 Botter::isValidBoneArray(const char* data, u32& type) const
{
	u32 iAnchor = *(u32*)(data + 0);
	if (!(iAnchor >= 0x3F7FFFFE && iAnchor <= 0x3F800000)) return -77;// ||  iAnchor == 0)) return -77;

	//u32 aOffs = 10;
	//if (!(iAnchor >= 0x3F7FFFFE && iAnchor <= 0x3F800000)) return -77;// ||  iAnchor == 0)) return -77;

	type = 0;
	i32 res=0;
	Bone* b = (Bone*)(data + BonePreOffset);
	for (u32 i = 0; i < g_numBones; i++, b++)
	{
		res = validPosition(b->pos);
		if (res != 0) break;
		res = validQuat(b->rot);
		if (res != 0) break;
		//if ( fabs(b->pos.w) > 0.01f ) return -95; // THIS HAPPENS!!
	}

	if ( res != 0 )
	{
		if ( iAnchor != 0x3F800000 ) return -78;
		Vec3 pos = *(Vec3*)(data + 0x3C);
		res = validPosition(pos);
		if (res != 0) return res;
		pos = *(Vec3*)(data + 0x3C + 16);
		res = validPosition(pos);
		if (res != 0) return res;
		type = 1;
	}

	// check distances of bones
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

bool Botter::nearZero(float f)
{
	return f >= -0.01f && f <= 0.01f;
	//return (f > -20.101f && f < 20.101f);
}

bool Botter::nearZero(Vec3 f)
{
	//return fabs(f.x)+fabs(f.y)+fabs(f.z) < 50;
	return f.length() < 20.f;
	//return nearZero(f.x) && nearZero(f.y) && nearZero(f.z);
}

bool Botter::isSmall(float f)
{
	return f > -50 && f < 50;
}

bool Botter::isSmall(Vec3 f)
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

bool Botter::isSimilar(Vec3 f)
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

bool Botter::zOrNorm(float f)
{
	return isnormal(f) || f==0.f;
}

bool Botter::isIntegral(float f)
{
	//return false;
//	return ((float)i32(f)) == f;
	//// only allow 3 decimals
	i64 iTemp = (i64)(f * 1000.f);
	float kd = (float)iTemp;
	float fd = float(kd * 0.001);
	return float(i32(fd)) == fd;
}

bool Botter::isValidInt(i32 i)
{
	return i >= -1000 && i <= 1000;
}

u32 Botter::insideWorld(float f)
{
	if ( f < -10000.f ) return -50;
	if ( f > 10000.f ) return -51;
//	if ( isIntegral(f) ) return -52;	INTEGRAL HAPPENS ON RESPAWNS!
	// if ( !zOrNorm(f) ) return -53;
	if ( !isnormal(f) ) return -54;
	return 0;
}

bool Botter::validUp(float f)
{
	return f >= -1500 && f < 3000 && isnormal(f); // zOrNorm(f);
	//return (f >= -50.f && f <= 1300.f) /* !isIntegral(f)*/ && zOrNorm(f);
}

u32 Botter::validPosition(const Vec3& pos)
{
	// if ( nearZero(pos) ) return -6;
	Vec3 pos2D = pos;
	pos2D.z = 0.f;
	if ( nearZero(pos2D) ) return -6;
	//if ( fabs(pos2D.x) < 35 ) return -7;
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

u32 Botter::validQuat(const Vec4& q)
{
	float l = q.length();
	if ( !(l >= 0.85f && l <= 1.15f) ) return -100;
	if ( !isnormal(l) ) return -101;
	if ( !(zOrNorm(q.x) && zOrNorm(q.y) && zOrNorm(q.z) && zOrNorm(q.w)) ) return -102;
	return 0;
}

bool Botter::validVector(const Vec3& v)
{
	float l = v.length();
	return l >= 0.95f && l <= 1.05f && isnormal(l) && 
			zOrNorm(v.x) && zOrNorm(v.y) && zOrNorm(v.z);
}


