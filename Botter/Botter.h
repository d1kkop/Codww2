#pragma once

#include "Null.h"
#include "../Cww2/Source/Vec3.h"

using namespace Null;


const u32 g_numBones = 2;
extern u32 SleepTimeMs;


enum class Team
{
	Unknown,
	Own,
	Enemy
};

enum class MemoryState
{
	Find,
	Filter,
	Wait
};


struct Bone
{
	Vec4 pos;
	Vec4 rot;
};


// --------------- Velocity ------------------------------------------------------------------------------------------------------------------------------------


struct Velocity
{
	Velocity() : lastTs(0) 
	{
	//	vel = Vec3(1000, 1000, 1000);
	}

	void update(const Vec3& newPos);
	Vec3 velocity() const { return vel; }

private:
	float lastTs;
	Vec3 oldPos;
	Vec3 vel;
};


// --------------- GameCamera ------------------------------------------------------------------------------------------------------------------------------------


struct GameCamera
{
	GameCamera() : entry(nullptr), valid(false) { }

	template <typename T>
	T at(u32 offset) const { return *(T*)(entry->value.ptr + offset); }

	void update();
	Vec3 getPosition()		const { return at<Vec3>(0);  }
	Vec3 getFacing()		const { return at<Vec3>(16); }
	Vec3 getHeadPos()		const { return at<Vec3>(32); }
	Vec3 getUpVec()			const { return at<Vec3>(68); } // 44
	bool isAlive()			const { return at<bool>(80); }
	Vec3 getVelocity()		const { return vel.velocity(); }

public:
	const MemEntry* entry;
	bool valid;

private:
	Velocity vel;
};


// --------------- BoneArray ------------------------------------------------------------------------------------------------------------------------------------


struct IBoneArray
{
	virtual u32 getId() const = 0;
	virtual void update() = 0;
	virtual Vec3 computeDirTo(const Vec3& camPos, float& distOut) const = 0;
	virtual Vec3 getPosition() const = 0;
	virtual Vec3 getTargetPosition() const = 0;
	virtual float getAngle() const = 0;
	virtual Vec3 getVelocity() const = 0;
	virtual u32 getTeam() const = 0;
	virtual bool isValid() const = 0;
	virtual bool inView() const = 0;
	virtual const char* getAddress() const = 0;
	virtual u32 numBones() const = 0;
	virtual Vec3 getBonePosition(u32 boneIdx) const = 0;
	virtual Vec4 getBoneRotation(u32 boneIdx) const = 0;
	virtual void updateDynamicOffset(u32 offset) = 0;
	virtual void setAlly(bool isAlly) = 0;
	virtual bool isAlly() const = 0;
	virtual float distMoved() const = 0;
	virtual const MemEntry* getEntry() const = 0;
};


struct BoneArray2 : public IBoneArray
{
public:
	BoneArray2() : missile(false), valid(false), inview(false), id(-1), memEntry(nullptr),
		lastInViewCheckTs(0), isMoving(false), startedMovingTs(0), numTimesMoved(0),
		lastKnownMovedBoneIdx(0), dynOffset(0), ally(false), distTravalled(0), lastUpdateTimeTs(0)
	{
	}

	void update() override;
	void updateInView(float tNow);
	void updateValidation(float tNow, float dt);
	void updateInvalidate(float tNow);
	void updateTargetPoint();

	bool didValidMove() const;

	u32 getId() const override { return id; }
	Vec3 computeDirTo(const Vec3& camPos, float& distOut) const override;
	Vec3 getTargetPosition() const override;
	Vec3 getPosition() const override;
	Vec3 getVelocity() const override;
	float getAngle() const override;
	u32 getTeam() const override;
	bool isValid() const override;
	bool inView() const override;
	const char* getAddress() const override;
	u32 numBones() const override { return g_numBones; }
	Vec3 getBonePosition(u32 boneIdx) const override;
	Vec4 getBoneRotation(u32 boneIdx) const override;
	void updateDynamicOffset(u32 offset) override;
	void setAlly(bool isAlly) override { ally = isAlly; }
	bool isAlly() const override { return ally; }
	float distMoved() const override { return distTravalled; }
	const MemEntry* getEntry() const override { return memEntry; }

	float lastUpdateTimeTs;
	bool missile;
	bool valid;
	bool inview;
	u32 id;
	u32 lastKnownMovedBoneIdx;
	const MemEntry* memEntry;
	Velocity vel;
	bool ally;

	// validate if moving
	float lastInViewCheckTs;
	float startedMovingTs;
	bool isMoving;
	u32 numTimesMoved;
	u32 dynOffset;

	Vec3 targetPos;
	float distTravalled;

	Array<Vec3> prevBones;
};


// --------------- Botter ------------------------------------------------------------------------------------------------------------------------------------

class Botter
{
public:
	Botter();
	~Botter();
	bool valid() const { return m_scanner.getHandle() != nullptr; }

	void cameraSearch();		// seperate thread
	void boneArraySearch();		// seperate thread
	bool gameTick(u32 tickIdx, float dt);	// main thread
	
	void findCamera();
	void filterCamera();

	void findBoneArrays();
	void filterBoneArrays();

	void aim(float tNow, float dt);
	void updateBoneArrays();
	void updateCamera();
	void updateAllies();

	void reloadConfig();


	u32 numBoneArrays() const { return (u32)m_boneArrays.size(); }
	u32 numValidBoneArrays() const { return m_numValidBoneArrays; }
	u32 numInViewBoneArrays() const { return m_numInViewBoneArrays; }
	u32 numEntriesInMemory() const
	{ 
		std::lock_guard<std::mutex> lk( m_boneArrayMutex );
		if (!m_gameBoneArrayScan) return 0;
		return (u32)m_gameBoneArrayScan->numEntries();
	}

	IBoneArray* fetchCurrentTarget() const;
	void printCameraStats() const;
	void printCameraStats(Vec3 p, Vec3 f, Vec3 up, Vec3 head, Vec3 vel) const;
	void printBoneArrays() const;
	void printBoneArraysSimple() const;
	void printBoneArray(const IBoneArray* ba) const;
	void printBoneArrayRaw(const char* data) const;

	// higher level type
	i32 isValidCamera(const char* data) const;
	u32 isValidPlayerBot(const char* data) const;
	u32 isValidBoneArray(const char* data) const;

	// generic
	bool nearZero(float f) const;
	bool nearZero(Vec3 f) const;
	bool isSmall(float f) const;
	bool isSmall(Vec3 f) const;
	bool isSimilar(Vec3 f) const;
	bool zOrNorm(float f) const;
	bool isIntegral(float f) const;
	bool isValidInt(i32 i) const;
	u32 insideWorld(float f) const;
	bool validUp(float f) const;
	u32 validPosition(const Vec3& p) const;
	u32 validQuat(const Vec4& q) const;
	bool validVector(const Vec3& v) const;

	static void log(const String& msg);

	// in thread
	volatile bool m_closing;
	std::thread m_cameraThread;
	std::thread m_boneArrayThread;
	Scanner3 m_scanner;
	Scan* m_camScan;
	Scan* m_boneArrayScan;
	std::atomic< MemoryState > m_cameraState;
	std::atomic< MemoryState > m_boneArrayState;

	// in main thread
	mutable std::mutex m_boneArrayMutex;
	mutable std::mutex m_camMutex;
	Array<IBoneArray*>  m_boneArrays;
	GameCamera m_gameCam;
	
	Scan* m_gameCamScan;
	Scan* m_gameBoneArrayScan;
	u64 m_gameCamStateInvalidTs;
	u64 m_gameBoneStateInvalidTs;
	u32 m_numValidBoneArrays;
	u32 m_numInViewBoneArrays;
	u32 m_uniqueBoneArrayId;
	u32 m_numBoneRestartTicks;

	const char* m_target; // loose coupling
	Vec3 m_lastTargetPos;
	Vec3 m_lastViewDir;
	float m_lastTargetTs;
	bool m_isFocussing;

	// loggin
	File m_log;
};
