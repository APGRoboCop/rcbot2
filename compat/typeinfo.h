// Compatibility wrapper: modern MSVC removed <typeinfo.h>, only <typeinfo> exists.
// This allows hl2sdk headers (e.g. memalloc.h) to compile without SDK modifications. [APG]RoboCop[CL]
#ifndef TYPEINFO_H
#define TYPEINFO_H

#include <typeinfo>

#endif // TYPEINFO_H
