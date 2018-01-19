#include "TimeUtil.h"


namespace Null
{
	using namespace std;
	using namespace chrono;

	static u64 g_start_time_nano  = time_absolute_nano();
	static u64 g_start_time_micro = time_absolute_micro();
	static u64 g_start_time_milli = time_absolute_milli();


	u64 time_absolute_nano()
	{
		return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}

	u64 time_absolute_micro()
	{
		return duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}

	u64 time_absolute_milli()
	{
		return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}

	u64 time_now_nano()
	{
		return time_absolute_nano() - g_start_time_nano;
	}

	u64 time_now_micro()
	{
		return time_absolute_micro() - g_start_time_micro;
	}

	u64 time_now_milli()
	{
		return time_absolute_milli() - g_start_time_milli;
	}

	void sleep_nano(u64 duration)
	{
		if (duration==0) return;
		std::this_thread::sleep_for(std::chrono::nanoseconds(duration));
	}

	void sleep_micro(u64 duration)
	{
		if (duration==0) return;
		std::this_thread::sleep_for(std::chrono::microseconds(duration));
	}

	void sleep_milli(u64 duration)
	{
		if (duration==0) return;
		std::this_thread::sleep_for(std::chrono::milliseconds(duration));
	}

	float to_seconds(u64 milliseconds)
	{
		return (float)to_secondsD(milliseconds);
	}

	double to_secondsD(u64 milliseconds)
	{
		return ((double)milliseconds) * 0.001;
	}

}
