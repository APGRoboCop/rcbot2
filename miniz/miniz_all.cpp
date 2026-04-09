// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/*
 * miniz_all.cpp - Single C++ translation unit wrapper for miniz.
 *
 * The AMBuild/MMS build infrastructure applies C++-only compiler flags
 * (e.g. -std=c++17, -Wno-overloaded-virtual) globally to all source files.
 * Compiling .c files with these flags causes errors under -Werror on GCC.
 *
 * This wrapper includes all miniz .c files so they are compiled as C++.
 * All miniz sources already use extern "C" linkage guards, so the resulting
 * symbols have correct C linkage.
 */

#include "miniz.c"
#include "miniz_tdef.c"
#include "miniz_tinfl.c"
#include "miniz_zip.c"