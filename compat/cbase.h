// Stub: prevents game/server/cbase.h from being pulled in during debug builds.
// In debug mode, game/shared/ehandle.h conditionally includes cbase.h when
// _DEBUG and GAME_DLL are defined. Without this stub, it resolves to
// game/server/cbase.h (via the game/server include path), which cascades into
// util.h, variant_t.h, takedamageinfo.h, etc. [APG]RoboCop[CL]
#ifndef CBASE_H
#define CBASE_H

#endif
