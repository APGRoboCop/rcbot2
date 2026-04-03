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
#ifndef __BOT_COMPRESS_H__
#define __BOT_COMPRESS_H__

#include <cstddef>
#include <cstdint>

// Compression magic number to identify compressed files: "RCz\x01"
constexpr uint32_t RCBOT_COMPRESS_MAGIC = 0x017A4352;

// Header written at the start of compressed files
#pragma pack(push, 1)
struct rcbot_compress_header_t
{
	uint32_t magic;              // RCBOT_COMPRESS_MAGIC
	uint32_t uncompressed_size;  // original data size in bytes
};
#pragma pack(pop)

// Writes raw data to a file using miniz deflate compression.
// Returns true on success.
bool RCBot_CompressedSave(const char* filename, const void* pData, std::size_t dataSize);

// Reads a file that may be compressed (auto-detects via magic header).
// Writes up to expectedSize bytes into pOutData.
// Returns true on success.
bool RCBot_CompressedLoad(const char* filename, void* pOutData, std::size_t expectedSize);

#endif
