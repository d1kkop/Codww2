#include "FileUtil.h"


namespace Null
{

	bool file_error(File f)
	{
		return ferror(f) != 0;
	}

	File file_open(const char* filename, const char* mode)
	{
		File f = nullptr;
	#if NULL_WINDOWS
		fopen_s(&f, filename, mode);
	#else
		f = fopen(filename, mode);
	#endif
		return f;
	}

	bool file_write(File f, const void* data, u64 length)
	{
		return fwrite(data, length, 1, f) == 1;
	}

	bool file_read(File f, void* data, u64 length)
	{
		return fread(data, length, 1, f) == 1;
	}

	void file_flush(File f)
	{
		fflush(f);
	}

	void file_close(File f)
	{
		fclose(f);
	}

	void file_remove(const char* filename)
	{
		::remove( filename );
	}

	void file_print(File f, const String& formattedMsg)
	{
	#if NULL_WINDOWS
		fprintf_s(f, "%s", formattedMsg.c_str());
	#else
		fprintf(f, "%s", formattedMsg.c_str());
	#endif
	}

	String file_scanStr(File f)
	{
		char buffer[NULL_BUFF_SIZE];
		buffer[0] = '\0';
	#if NULL_WINDOWS
		fscanf_s(f, "%s", buffer, NULL_BUFF_SIZE-1);
	#else
		fscanf(f, "%s", buffer);
	#endif
		return buffer;
	}

	u32 file_scanU32(File f)
	{
		u32 u = 0;
	#if NULL_WINDOWS
		fscanf_s(f, "%lu", &u);
	#else
		fscanf(f, "%lu", &u);
	#endif
		return u;
	}

	i32 file_scanI32(File f)
	{
		u32 i = 0;
	#if NULL_WINDOWS
		fscanf_s(f, "%li", &i);
	#else
		fscanf(f, "%li", &i);
	#endif
		return i;
	}

	u64 file_scanU64(File f)
	{
		u64 i = 0;
	#if NULL_WINDOWS
		fscanf_s(f, "%llu", &i);
	#else
		fscanf(f, "%llu", &i);
	#endif
		return i;
	}

	i64 file_scanI64(File f)
	{
		i64 i = 0;
	#if NULL_WINDOWS
		fscanf_s(f, "%lli", &i);
	#else
		fscanf(f, "%lli", &i);
	#endif
		return i;
	}

	float file_scanFloat(File f)
	{
		float val = 0.f;
	#if NULL_WINDOWS
		fscanf_s(f, "%f", &val);
	#else
		fscanf(f, "%f", &val);
	#endif
		return val;
	}

	double file_scanDouble(File f)
	{
		double val = 0;
	#if NULL_WINDOWS
		fscanf_s(f, "%lf", &val);
	#else
		fscanf(f, "%lf", &val);
	#endif
		return val;
	}

}
