// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
#include "bot_compress.h"
#include "rcbot/logging.h"

#include "miniz.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

bool RCBot_CompressedSave(const char* filename, const void* pData, const std::size_t dataSize)
{
	mz_ulong compBound = mz_compressBound(static_cast<mz_ulong>(dataSize));
	unsigned char* pCompressed = static_cast<unsigned char*>(std::malloc(compBound));

	if (!pCompressed)
	{
		logger->Log(LogLevel::ERROR, "RCBot_CompressedSave: malloc failed for '%s'", filename);
		return false;
	}

	const int status = mz_compress2(pCompressed, &compBound,
		static_cast<const unsigned char*>(pData),
		static_cast<mz_ulong>(dataSize), MZ_BEST_COMPRESSION);

	if (status != MZ_OK)
	{
		logger->Log(LogLevel::ERROR, "RCBot_CompressedSave: compression failed (%d) for '%s'", status, filename);
		std::free(pCompressed);
		return false;
	}

	FILE* fp = std::fopen(filename, "wb");

	if (!fp)
	{
		logger->Log(LogLevel::ERROR, "RCBot_CompressedSave: can't open '%s' for writing", filename);
		std::free(pCompressed);
		return false;
	}

	rcbot_compress_header_t hdr;
	hdr.magic = RCBOT_COMPRESS_MAGIC;
	hdr.uncompressed_size = static_cast<uint32_t>(dataSize);

	std::fwrite(&hdr, sizeof(hdr), 1, fp);
	std::fwrite(pCompressed, 1, compBound, fp);
	std::fclose(fp);

	std::free(pCompressed);

	logger->Log(LogLevel::INFO, "RCBot_CompressedSave: saved '%s' (%u -> %lu bytes)",
		filename, static_cast<unsigned>(dataSize), static_cast<unsigned long>(compBound));

	return true;
}

bool RCBot_CompressedLoad(const char* filename, void* pOutData, const std::size_t expectedSize)
{
	FILE* fp = std::fopen(filename, "rb");

	if (!fp)
	{
		logger->Log(LogLevel::ERROR, "RCBot_CompressedLoad: can't open '%s' for reading", filename);
		return false;
	}

	// Get file size
	std::fseek(fp, 0, SEEK_END);
	const long fileSize = std::ftell(fp);
	std::fseek(fp, 0, SEEK_SET);

	if (fileSize < 0)
	{
		std::fclose(fp);
		return false;
	}

	// Read magic to detect if compressed
	uint32_t magic = 0;

	if (static_cast<std::size_t>(fileSize) >= sizeof(rcbot_compress_header_t))
	{
		std::fread(&magic, sizeof(magic), 1, fp);
		std::fseek(fp, 0, SEEK_SET);
	}

	if (magic == RCBOT_COMPRESS_MAGIC)
	{
		// Compressed file
		rcbot_compress_header_t hdr;
		std::fread(&hdr, sizeof(hdr), 1, fp);

		if (hdr.uncompressed_size != static_cast<uint32_t>(expectedSize))
		{
			logger->Log(LogLevel::ERROR, "RCBot_CompressedLoad: size mismatch in '%s' (expected %u, got %u)",
				filename, static_cast<unsigned>(expectedSize), hdr.uncompressed_size);
			std::fclose(fp);
			return false;
		}

		const std::size_t compressedSize = static_cast<std::size_t>(fileSize) - sizeof(hdr);
		unsigned char* pCompressed = static_cast<unsigned char*>(std::malloc(compressedSize));

		if (!pCompressed)
		{
			std::fclose(fp);
			return false;
		}

		std::fread(pCompressed, 1, compressedSize, fp);
		std::fclose(fp);

		mz_ulong destLen = static_cast<mz_ulong>(expectedSize);
		const int status = mz_uncompress(
			static_cast<unsigned char*>(pOutData), &destLen,
			pCompressed, static_cast<mz_ulong>(compressedSize));

		std::free(pCompressed);

		if (status != MZ_OK)
		{
			logger->Log(LogLevel::ERROR, "RCBot_CompressedLoad: decompression failed (%d) for '%s'", status, filename);
			return false;
		}

		logger->Log(LogLevel::INFO, "RCBot_CompressedLoad: loaded '%s' (compressed %lu -> %u bytes)",
			filename, static_cast<unsigned long>(compressedSize), static_cast<unsigned>(expectedSize));

		return true;
	}

	// Uncompressed (legacy) file - read directly
	if (static_cast<std::size_t>(fileSize) != expectedSize)
	{
		logger->Log(LogLevel::ERROR, "RCBot_CompressedLoad: uncompressed size mismatch in '%s' (expected %u, got %ld)",
			filename, static_cast<unsigned>(expectedSize), fileSize);
		std::fclose(fp);
		return false;
	}

	std::fread(pOutData, 1, expectedSize, fp);
	std::fclose(fp);

	logger->Log(LogLevel::INFO, "RCBot_CompressedLoad: loaded uncompressed '%s' (%u bytes)",
		filename, static_cast<unsigned>(expectedSize));

	return true;
}
