#include "build_info.h"
#include "rcbot/rb-platform.h"

// generated with versioning/generate.py, AMBuild handles this
#include <build_version_auto.h>

const char* build_info::authors = "Cheeseh, RoboCop, nosoop, pongo1231, caxanga334";
const char* build_info::url = "http://rcbot.bots-united.com/";

const char* build_info::long_version = "APGRoboCop/rcbot2@" GIT_COMMIT_HASH;
const char* build_info::short_version = "2.0-alpha9 " RCBOT_ARCH_TAG " (apg)-" GIT_COMMIT_SHORT_HASH;

const char* build_info::date = __DATE__;