#include "StrUtil.h"


namespace Null
{
	u64 str_length(const char* str)
	{
		return strlen(str);
	}

	bool str_equal(const char* left, const char* right)
	{
		return strcmp(left, right) == 0;
	}

	void str_copy(char* dst, u64 dstSize, const char* data)
	{
	#if NULL_WINDOWS
		strcpy_s(dst, dstSize, data);
	#else
		strcpy(dst, data);
	#endif
	}

	char* str_new(const char* src)
	{
		u64 len = str_length(src)+1;
		// char has no destructor so either case: delete [] or free will do
		char *c = new char[len];
		str_copy(c, len, src);
		return c;
	}

	char* str_new(const String& fromSrc)
	{
		return str_new(fromSrc.c_str());
	}

	char* str_replace(char* dst, u64 newSize, const char* newData)
	{
		// char has no destructor so either case: delete [] or free will do
		dst = (char*)realloc(dst, newSize);
		str_copy(dst, newSize, newData);
		return dst;
	}

	bool str_contains(const char* src, const char* substr)
	{
		return strstr(src, substr) != nullptr;
	}

	bool str_contains(const String& src,const String& substr)
	{
		return str_contains(src.c_str(), substr.c_str());
	}

	void str_split(const String& source, const String& delims, Array<String>& out)
	{
		String current;
		for (char c : source)
		{
			u32 j=0;
			for (; j<delims.size(); j++)
			{
				if (c == delims[j])
				{
					if (!current.empty()) out.emplace_back(current);
					current.clear();
					break;
				}
			}
			if (j == delims.size()) current += c;
		}
		if (!current.empty()) out.emplace_back(current);
	}

	void str_split(const String& source, const String& delims, const Function<void(const String& subStr)>& cb)
	{
		assert(cb);
		String current;
		for (char c : source)
		{
			u32 j=0;
			for (; j<delims.size(); j++)
			{
				if (c == delims[j])
				{
					if (!current.empty()) cb(current);
					current.clear();
					break;
				}
			}
			if (j == delims.size()) current += c;
		}
		if (!current.empty()) cb(current);
	}

	i32 str_toI32(const char* src, u32 base)
	{
		return strtol(src, nullptr, base);
	}

	i32 str_toI32(const String& src, u32 base)
	{
		return str_toI32(src.c_str(), base);
	}

	u32 str_toU32(const char* src, u32 base)
	{
		return strtoul(src, nullptr, base);
	}

	u32 str_toU32(const String& src, u32 base)
	{
		return str_toU32(src.c_str(), base);
	}

	float str_toFloat(const char* src)
	{
		return strtof(src, nullptr);
	}

	float str_toFloat(const String& src)
	{
		return str_toFloat(src.c_str());
	}

	double str_toDouble(const char* src)
	{
		return strtod(src, nullptr);
	}

	double str_toDouble(const String& src)
	{
		return str_toDouble(src.c_str());
	}

	void* str_toPtr(const char* src)
	{
		return (void*)strtoull(src, nullptr, 0);
	}

	void* str_toPtr(const String& src)
	{
		return str_toPtr(src.c_str());
	}

	String str_format(const char* fmt, ...)
	{
		char buff[NULL_BUFF_SIZE];
		va_list myargs;
		va_start(myargs, fmt);
	#if NULL_WINDOWS
		vsprintf_s(buff, NULL_BUFF_SIZE, fmt, myargs);
	#else
		vsprintf(buff, fmt, myargs);
	#endif
		va_end(myargs);
		return buff;
	}

}
