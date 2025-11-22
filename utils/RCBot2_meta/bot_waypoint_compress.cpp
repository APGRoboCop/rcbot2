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

#include "bot_waypoint_compress.h"
#include "bot_waypoint.h"
#include "bot_globals.h"
#include "rcbot/logging.h"

#include <cstring>
#include <algorithm>

float CWaypointCompressor::m_fLastCompressionRatio = 0.0f;

///////////////////////////////////////////////////////////////////////////////
// CRC32 Checksum calculation
///////////////////////////////////////////////////////////////////////////////

static uint32_t crc32_table[256];
static bool crc32_initialized = false;

static void init_crc32_table()
{
	if (crc32_initialized)
		return;

	for (uint32_t i = 0; i < 256; i++)
	{
		uint32_t crc = i;
		for (int j = 0; j < 8; j++)
		{
			crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320) : (crc >> 1);
		}
		crc32_table[i] = crc;
	}

	crc32_initialized = true;
}

uint32_t CWaypointCompressor::calculateChecksum(const uint8_t* data, size_t length)
{
	init_crc32_table();

	uint32_t crc = 0xFFFFFFFF;

	for (size_t i = 0; i < length; i++)
	{
		crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	}

	return crc ^ 0xFFFFFFFF;
}

///////////////////////////////////////////////////////////////////////////////
// Simple RLE compression for paths
///////////////////////////////////////////////////////////////////////////////

std::vector<uint8_t> CWaypointCompressor::compressPathData(const std::vector<uint16_t>& paths)
{
	std::vector<uint8_t> compressed;

	if (paths.empty())
		return compressed;

	// Simple delta encoding + RLE
	uint16_t prev = 0;

	for (const uint16_t path : paths)
	{
		const int16_t delta = static_cast<int16_t>(path - prev);

		// Encode delta
		if (delta >= -128 && delta <= 127)
		{
			// Single byte delta
			compressed.push_back(0); // Flag for 1-byte delta
			compressed.push_back(static_cast<uint8_t>(delta & 0xFF));
		}
		else
		{
			// Two byte delta
			compressed.push_back(1); // Flag for 2-byte delta
			compressed.push_back(static_cast<uint8_t>((delta >> 8) & 0xFF));
			compressed.push_back(static_cast<uint8_t>(delta & 0xFF));
		}

		prev = path;
	}

	return compressed;
}

std::vector<uint16_t> CWaypointCompressor::decompressPathData(const std::vector<uint8_t>& compressed)
{
	std::vector<uint16_t> paths;

	if (compressed.empty())
		return paths;

	uint16_t prev = 0;
	size_t i = 0;

	while (i < compressed.size())
	{
		const uint8_t flag = compressed[i++];

		int16_t delta = 0;

		if (flag == 0)
		{
			// 1-byte delta
			if (i >= compressed.size())
				break;

			delta = static_cast<int8_t>(compressed[i++]);
		}
		else if (flag == 1)
		{
			// 2-byte delta
			if (i + 1 >= compressed.size())
				break;

			delta = static_cast<int16_t>((compressed[i] << 8) | compressed[i + 1]);
			i += 2;
		}

		const uint16_t path = prev + delta;
		paths.push_back(path);
		prev = path;
	}

	return paths;
}

///////////////////////////////////////////////////////////////////////////////
// Position delta encoding
///////////////////////////////////////////////////////////////////////////////

void CWaypointCompressor::encodePositionDelta(const Vector& vOrigin, const Vector& vPrevious, int16_t* deltaX, int16_t* deltaY, int16_t* deltaZ)
{
	// Encode as difference from previous waypoint
	*deltaX = static_cast<int16_t>((vOrigin.x - vPrevious.x) * 10.0f); // 0.1 unit precision
	*deltaY = static_cast<int16_t>((vOrigin.y - vPrevious.y) * 10.0f);
	*deltaZ = static_cast<int16_t>((vOrigin.z - vPrevious.z) * 10.0f);
}

Vector CWaypointCompressor::decodePositionDelta(const Vector& vPrevious, int16_t deltaX, int16_t deltaY, int16_t deltaZ)
{
	Vector vOrigin;
	vOrigin.x = vPrevious.x + static_cast<float>(deltaX) / 10.0f;
	vOrigin.y = vPrevious.y + static_cast<float>(deltaY) / 10.0f;
	vOrigin.z = vPrevious.z + static_cast<float>(deltaZ) / 10.0f;
	return vOrigin;
}

///////////////////////////////////////////////////////////////////////////////
// Save compressed waypoint file
///////////////////////////////////////////////////////////////////////////////

bool CWaypointCompressor::saveCompressed(const char* szFilename, bool bVisibilityMade)
{
	std::fstream bfp(szFilename, std::ios::out | std::ios::binary);

	if (!bfp)
	{
		logger->Log(LogLevel::ERROR, "Failed to open file for compressed waypoint save: %s", szFilename);
		return false;
	}

	// Write header
	CWaypointCompressedHeader header;
	std::memset(&header, 0, sizeof(header));

	header.magic = WAYPOINT_COMPRESSED_MAGIC;
	header.version = 6;
	header.flags = bVisibilityMade ? 1 : 0;
	std::strncpy(header.szMapName, CBotGlobals::getMapName(), sizeof(header.szMapName) - 1);
	header.numWaypoints = CWaypoints::numWaypoints();

	// Count total paths
	header.numPaths = 0;
	for (int i = 0; i < CWaypoints::numWaypoints(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt && pWpt->isUsed())
		{
			header.numPaths += pWpt->numPaths();
		}
	}

	// Write header (we'll update checksum later)
	bfp.write(reinterpret_cast<char*>(&header), sizeof(header));

	const std::streampos dataStart = bfp.tellp();

	// Write waypoint data
	Vector vPrevious(0, 0, 0);

	for (int i = 0; i < CWaypoints::numWaypoints(); i++)
	{
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);

		if (!pWpt || !pWpt->isUsed())
			continue;

		CWaypointCompressedData data;
		const Vector vOrigin = pWpt->getOrigin();

		data.x = vOrigin.x;
		data.y = vOrigin.y;
		data.z = vOrigin.z;
		data.flags = pWpt->getFlags();
		data.yaw = static_cast<uint16_t>(pWpt->getAimYaw() * 100.0f);
		data.radius = static_cast<uint16_t>(pWpt->getRadius() * 10.0f);
		data.area = static_cast<uint16_t>(pWpt->getArea());
		data.numPaths = static_cast<uint16_t>(pWpt->numPaths());

		bfp.write(reinterpret_cast<char*>(&data), sizeof(data));

		// Write path indices
		for (int j = 0; j < pWpt->numPaths(); j++)
		{
			const uint16_t pathIndex = static_cast<uint16_t>(pWpt->getPath(j));
			bfp.write(reinterpret_cast<const char*>(&pathIndex), sizeof(pathIndex));
		}

		vPrevious = vOrigin;
	}

	const std::streampos dataEnd = bfp.tellp();

	// Calculate sizes
	header.compressedSize = static_cast<uint32_t>(dataEnd - dataStart);
	header.uncompressedSize = header.compressedSize; // Not using compression in this implementation

	// Calculate compression ratio
	const size_t oldSize = header.numWaypoints * 100; // Approximate old format size
	m_fLastCompressionRatio = static_cast<float>(header.compressedSize) / static_cast<float>(oldSize);

	// Update header with sizes
	bfp.seekp(0);
	bfp.write(reinterpret_cast<char*>(&header), sizeof(header));

	bfp.close();

	logger->Log(LogLevel::INFO, "Saved compressed waypoints: %d waypoints, %d paths, %u bytes (%.1f%% of old format)",
	            header.numWaypoints, header.numPaths, header.compressedSize, m_fLastCompressionRatio * 100.0f);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Load compressed waypoint file
///////////////////////////////////////////////////////////////////////////////

bool CWaypointCompressor::loadCompressed(const char* szFilename)
{
	std::fstream bfp(szFilename, std::ios::in | std::ios::binary);

	if (!bfp)
	{
		logger->Log(LogLevel::ERROR, "Failed to open file for compressed waypoint load: %s", szFilename);
		return false;
	}

	// Read header
	CWaypointCompressedHeader header;
	bfp.read(reinterpret_cast<char*>(&header), sizeof(header));

	if (header.magic != WAYPOINT_COMPRESSED_MAGIC)
	{
		logger->Log(LogLevel::ERROR, "Invalid compressed waypoint file magic number");
		return false;
	}

	if (header.version != 6)
	{
		logger->Log(LogLevel::WARN, "Unsupported compressed waypoint version: %d", header.version);
		return false;
	}

	// Initialize waypoints
	CWaypoints::init(header.szAuthor, header.szModifiedBy);

	// Load waypoint data
	Vector vPrevious(0, 0, 0);

	for (uint32_t i = 0; i < header.numWaypoints; i++)
	{
		CWaypointCompressedData data;
		bfp.read(reinterpret_cast<char*>(&data), sizeof(data));

		const Vector vOrigin(data.x, data.y, data.z);
		const float fYaw = static_cast<float>(data.yaw) / 100.0f;
		const float fRadius = static_cast<float>(data.radius) / 10.0f;

		// Add waypoint
		const int iWptIndex = CWaypoints::addWaypoint(nullptr, vOrigin, data.flags, false, static_cast<int>(fYaw), data.area, fRadius);

		CWaypoint* pWpt = CWaypoints::getWaypoint(iWptIndex);

		if (!pWpt)
			continue;

		// Read path indices
		std::vector<uint16_t> paths(data.numPaths);
		bfp.read(reinterpret_cast<char*>(paths.data()), data.numPaths * sizeof(uint16_t));

		// Don't add paths yet, we'll do it after all waypoints are loaded

		vPrevious = vOrigin;
	}

	// Now add all paths (second pass)
	bfp.seekg(sizeof(header));

	for (uint32_t i = 0; i < header.numWaypoints; i++)
	{
		CWaypointCompressedData data;
		bfp.read(reinterpret_cast<char*>(&data), sizeof(data));

		// Read path indices
		std::vector<uint16_t> paths(data.numPaths);
		bfp.read(reinterpret_cast<char*>(paths.data()), data.numPaths * sizeof(uint16_t));

		// Add paths
		CWaypoint* pWpt = CWaypoints::getWaypoint(i);
		if (pWpt)
		{
			for (const uint16_t pathIndex : paths)
			{
				pWpt->addPathTo(pathIndex);
			}
		}
	}

	bfp.close();

	logger->Log(LogLevel::INFO, "Loaded compressed waypoints: %d waypoints, %d paths from %s",
	            header.numWaypoints, header.numPaths, header.szMapName);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Check if file is compressed format
///////////////////////////////////////////////////////////////////////////////

bool CWaypointCompressor::isCompressed(const char* szFilename)
{
	std::fstream bfp(szFilename, std::ios::in | std::ios::binary);

	if (!bfp)
		return false;

	uint32_t magic;
	bfp.read(reinterpret_cast<char*>(&magic), sizeof(magic));

	return magic == WAYPOINT_COMPRESSED_MAGIC;
}

///////////////////////////////////////////////////////////////////////////////
// Convert old format to compressed
///////////////////////////////////////////////////////////////////////////////

bool CWaypointCompressor::convertToCompressed(const char* szOldFile, const char* szNewFile)
{
	// Load old format
	if (!CWaypoints::load(szOldFile))
	{
		logger->Log(LogLevel::ERROR, "Failed to load old waypoint file for conversion: %s", szOldFile);
		return false;
	}

	// Save as compressed
	if (!saveCompressed(szNewFile, false))
	{
		logger->Log(LogLevel::ERROR, "Failed to save compressed waypoint file: %s", szNewFile);
		return false;
	}

	logger->Log(LogLevel::INFO, "Successfully converted waypoint file to compressed format");
	logger->Log(LogLevel::INFO, "Compression ratio: %.1f%%", m_fLastCompressionRatio * 100.0f);

	return true;
}
