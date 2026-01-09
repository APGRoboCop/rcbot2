/*
 *    This file is part of RCBot.
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
 */

#ifndef __RCBOT_NDEBUGOVERLAY_H__
#define __RCBOT_NDEBUGOVERLAY_H__

#include "engine/ivdebugoverlay.h"

// Null-safe debug overlay - does nothing but satisfies the interface
// Used when IVDebugOverlay interface isn't available on older engines
class CNullDebugOverlay : public IVDebugOverlay
{
public:
	void AddEntityTextOverlay(int ent_index, int line_offset, float duration, int r, int g, int b, int a, const char *format, ...) override {}
	void AddBoxOverlay(const Vector& origin, const Vector& mins, const Vector& max, QAngle const& orientation, int r, int g, int b, int a, float duration) override {}
	void AddTriangleOverlay(const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest, float duration) override {}
	void AddLineOverlay(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration) override {}
	void AddTextOverlay(const Vector& origin, float duration, const char *format, ...) override {}
	void AddTextOverlay(const Vector& origin, int line_offset, float duration, const char *format, ...) override {}
	void AddScreenTextOverlay(float flXPos, float flYPos, float flDuration, int r, int g, int b, int a, const char *text) override {}
	void AddSweptBoxOverlay(const Vector& start, const Vector& end, const Vector& mins, const Vector& max, const QAngle & angles, int r, int g, int b, int a, float flDuration) override {}
	void AddGridOverlay(const Vector& origin) override {}
	int ScreenPosition(const Vector& point, Vector& screen) override { return 1; }
	int ScreenPosition(float flXPos, float flYPos, Vector& screen) override { return 1; }
	OverlayText_t *GetFirst(void) override { return nullptr; }
	OverlayText_t *GetNext(OverlayText_t *current) override { return nullptr; }
	void ClearDeadOverlays(void) override {}
	void ClearAllOverlays() override {}
	void AddTextOverlayRGB(const Vector& origin, int line_offset, float duration, float r, float g, float b, float alpha, const char *format, ...) override {}
	void AddTextOverlayRGB(const Vector& origin, int line_offset, float duration, int r, int g, int b, int a, const char *format, ...) override {}
	void AddLineOverlayAlpha(const Vector& origin, const Vector& dest, int r, int g, int b, int a, bool noDepthTest, float duration) override {}
	void AddBoxOverlay2(const Vector& origin, const Vector& mins, const Vector& max, QAngle const& orientation, const Color& faceColor, const Color& edgeColor, float duration) override {}
	void AddScreenTextOverlay2(float flXPos, float flYPos, int iLine, float flDuration, int r, int g, int b, int a, const char *text) override {}
};

// Global null overlay instance
extern CNullDebugOverlay g_NullDebugOverlay;

// Global debug overlay pointer (may point to real interface or null stub)
extern IVDebugOverlay* debugoverlay;

#endif // __RCBOT_NDEBUGOVERLAY_H__
