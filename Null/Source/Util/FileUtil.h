#pragma once

#include "../Common.h"


namespace Null
{
	using File = FILE*;

	bool	file_error(File f);
	File	file_open(const char* filename, const char* mode);
	bool	file_write(File f, const void* data, u64 length);
	bool	file_read(File f, void* data, u64 length);
	void	file_flush(File f);
	void	file_close(File f);
	void	file_remove(const char* filename);
	void	file_print(File f, const String& formattedMsg);
	String	file_scanStr(File f);
	u32		file_scanU32(File f);
	i32		file_scanI32(File f);
	u64		file_scanU64(File f);
	i64		file_scanI64(File f);
	float	file_scanFloat(File f);
	double	file_scanDouble(File f);

}
