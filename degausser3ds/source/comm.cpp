/*
 * comm.cpp
 *
 *  Created on: Jan 5, 2017
 *      Author: wangyu
 */
#include "comm.h"




#ifdef TEST
#include <stdint.h>
typedef uint8_t u8;   ///<  8-bit unsigned integer
typedef uint16_t u16; ///< 16-bit unsigned integer
typedef uint32_t u32; ///< 32-bit unsigned integer
typedef uint64_t u64; ///< 64-bit unsigned integer

typedef int8_t s8;   ///<  8-bit signed integer
typedef int16_t s16; ///< 16-bit signed integer
typedef int32_t s32; ///< 32-bit signed integer
typedef int64_t s64; ///< 64-bit signed integer

typedef s32 Result;

void testprintf(char* fmt, ...)
{
	va_list args;
	va_start(args,fmt);
	vprintf(fmt, args);
	va_end(args);
}
#else



void testprintf(char* fmt, ...)
{
	va_list args;
	va_start(args,fmt);
	myprintf(fmt);
	va_end(args);
}
#endif


// concatenate UTF16 strings
void ConcatUTF16(u16* dst, bool sanitizeFirst, ...)
{
	va_list vl;
	va_start(vl, sanitizeFirst);
	for (u16* src; (src = va_arg(vl, u16*)); sanitizeFirst = true)
	{
		for (u16 c; (c = *src); src++)
		{
			if (sanitizeFirst && (c == '\\' || c == '/' || c == '?' || c == '*' ||
				c == ':' || c == '|' || c == '"' || c == '<' || c == '>'))
			{
				c += 0xFEE0;
			}
			*dst++ = c;
		}
	}
	va_end(vl);
	*dst = 0;
}

Result gz_compress(void* dst, u32* dstLen, const void* src, u32 srcLen)
{
	memcpy(dst, gzip_header, 10);
	if (!(*dstLen = tdefl_compress_mem_to_mem(dst + 10, *dstLen - 18, src, srcLen, 0x300))) return -1; // ERROR COMPRESSING
	*dstLen += 18;
	*(u32*)(dst + *dstLen - 8) = mz_crc32(0, (const unsigned char*)src, srcLen);
	*(u32*)(dst + *dstLen - 4) = srcLen;
	return 0;
}

Result gz_decompress(void* dst, u32 dstLen, const void* src, u32 srcLen)
{
	if (memcmp(src, gzip_header, 10)) return -1; // GZIP HEADER ERROR
	if (dstLen != *(u32*)(src + srcLen - 4)) return -2; // UNEXPECTED LENGTH
	if (dstLen != tinfl_decompress_mem_to_mem(dst, *(u32*)(src + srcLen - 4), src + 10, srcLen - 18, 4)) return -3; // DECOMPRESS FAILED
	if (*(u32*)(src + srcLen - 8) != mz_crc32(0, (const unsigned char*) dst, dstLen)) return -4; // WRONG CRC32
	return 0;
}

Result gz_decompress2(void* dst, u32* dstLen, const void* src, u32 srcLen)
{
	*dstLen=*(u32*)(src + srcLen - 4);
	return gz_decompress(dst,*dstLen,src,srcLen);
}
