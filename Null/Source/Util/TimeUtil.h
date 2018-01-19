#pragma once

#include "../Common.h"


namespace Null
{
	/*	Returns time since epoch (1 Jan 1970) in nano, micro or milli seconds. */
	u64 time_absolute_nano();
	u64 time_absolute_micro();
	u64 time_absolute_milli();
	inline u64 time_absolute() { return time_absolute_milli(); }
	
	/*	Returns time since start of application in nano, micro or milli seconds. */
	u64 time_now_nano();
	u64 time_now_micro();
	u64 time_now_milli();
	inline u64 time_now() { return time_now_milli(); }

	/*	Returns immediately if duration is 0. */
	void sleep_nano(u64 duration);
	void sleep_micro(u64 duration);
	void sleep_milli(u64 duration);

	/*	In milliseconds. */
	inline void sleep(u64 duration) { sleep_milli(duration); }

	/* In seconds. Accuracy is microseconds. */
	inline void sleep(float duration) { sleep_micro(u64(duration*1e6f)); }

	/*	From milliseconds to seconds. 
		Note be careful with precision. Do not use on absoluate time, only on delta times. */	
	float to_seconds(u64 milliseconds);

	/*	From milliseconds to seconds. */
	double to_secondsD(u64 milliseconds);
}
