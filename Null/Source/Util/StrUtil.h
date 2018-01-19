#pragma once

#include "../Common.h"


namespace Null
{
	u64		str_length(const char* str);
	bool	str_equal(const char* left, const char* right);
	void	str_copy(char* dst, u64 dstSize, const char* data);
	char*	str_new(const char* fromSrc); // allocates a new char array and copies
	char*	str_new(const String& fromSrc); // allocates a new char array and copies
	char*	str_replace(char* dst, u64 newSize, const char* newData);
	bool	str_contains(const char* src, const char* substr);
	bool	str_contains(const String& src, const String& substr);
	void	str_split(const String& src, const String& delims, Array<String>& out);
	void	str_split(const String& src, const String& delims, const Function<void (const String& substr)>& cb);
	i32		str_toI32(const char* src, u32 base=10);
	i32		str_toI32(const String& src, u32 base=10);
	u32		str_toU32(const char* src, u32 base=10);
	u32		str_toU32(const String& src, u32 base=10);
	float	str_toFloat(const char* src);
	float	str_toFloat(const String& src);
	double	str_toDouble(const char* src);
	double	str_toDouble(const String& src);
	void*   str_toPtr(const char* src); // converts hex values to ptr (eg. 0x8812ab)
	void*   str_toPtr(const String& src); // converts hex values to ptr (eg. 0x8812ab)
	String  str_format(const char* format, ...);

	
	template <typename T>
	String to_str(const T& t)
	{
		return std::to_string(t);
	}
}
