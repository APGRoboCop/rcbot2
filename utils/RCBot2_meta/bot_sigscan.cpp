// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#if defined(_WIN64) || defined(_WIN32)
#include <Windows.h>
#else

#include <cstdint>
//#define byte uint8_t

//#include "shake.h" //bir3yk
#include "elf.h"

#define PAGE_SIZE 4096
#define PAGE_ALIGN_UP(x) (((x) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#endif

//#include "cbase.h"
//#include "baseentity.h"
#include "filesystem.h"
#include "interface.h"
#include "engine/iserverplugin.h"
#include "tier2/tier2.h"
#ifdef __linux__
//#include "shake.h"    //bir3yk
#endif
#include "eiface.h"
#include "bot_const.h"
#include "bot.h"
#include "bot_cvars.h"
#include "bot_fortress.h"
#include "bot_kv.h"
#include "bot_getprop.h"
#include "bot_sigscan.h"
#include "bot_mods.h"

#include <cstring>

CGameRulesObject *g_pGameRules_Obj = nullptr;
CCreateGameRulesObject *g_pGameRules_Create_Obj = nullptr;

void **g_pGameRules = nullptr;

void *GetGameRules()
{
	if (!g_pGameRules)
		return nullptr;

	return *g_pGameRules;
}

std::size_t CSignatureFunction::decodeHexString(unsigned char* buffer, const std::size_t maxlength, const char* hexstr)
{
	std::size_t written = 0;
	const std::size_t length = std::strlen(hexstr);

	for (std::size_t i = 0; i < length; i++)
	{
		if (written >= maxlength)
			break;
		buffer[written++] = hexstr[i];
		if (hexstr[i] == '\\' && hexstr[i + 1] == 'x')
		{
			if (i + 3 >= length)
				continue;
			// Get the hex part. 
			char s_byte[3];
			unsigned r_byte; // Use unsigned here
			s_byte[0] = hexstr[i + 2];
			s_byte[1] = hexstr[i + 3];
			s_byte[2] = '\0';
			// Read it as an integer 
			std::sscanf(s_byte, "%x", &r_byte);
			
			// Check if the value fits into a char
			if (r_byte <= UCHAR_MAX)
			{
			// Save the value 
				buffer[written - 1] = static_cast<unsigned char>(r_byte);
			}
			else
			{
				// Handle the error
			}
			// Adjust index 
			i += 3;
		}
	}

	return written;
}

bool CSignatureFunction::getLibraryInfo(const void *libPtr, DynLibInfo &lib)
{
	std::uintptr_t baseAddr;

	if (libPtr == nullptr)
	{
		return false;
	}

#if defined(_WIN64) || defined(_WIN32)

	MEMORY_BASIC_INFORMATION info;

	if (!VirtualQuery(libPtr, &info, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return false;
	}

	baseAddr = reinterpret_cast<std::uintptr_t>(info.AllocationBase);

	// All this is for our insane sanity checks :o 
	const IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(baseAddr);
	const IMAGE_NT_HEADERS* pe = reinterpret_cast<IMAGE_NT_HEADERS*>(baseAddr + static_cast<std::uintptr_t>(dos->e_lfanew));
	const IMAGE_FILE_HEADER* file = &pe->FileHeader;
	const IMAGE_OPTIONAL_HEADER* opt = &pe->OptionalHeader;

	// Check PE magic and signature 
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE || opt->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		return false;
	}

	// Check architecture, which is 32-bit/x86 right now
	// Should change this for 64-bit if Valve gets their act together

	if (file->Machine != IMAGE_FILE_MACHINE_I386)
	{
		return false;
	}

	//For our purposes, this must be a dynamic library 
	if ((file->Characteristics & IMAGE_FILE_DLL) == 0)
	{
		return false;
	}

	//Finally, we can do this
	lib.memorySize = static_cast<std::size_t>(opt->SizeOfImage);

#else
	Dl_info info;
	Elf32_Ehdr *file;
	Elf32_Phdr *phdr;
	uint16_t phdrCount;

	if (!dladdr(libPtr, &info))
	{
		return false;
	}

	if (!info.dli_fbase || !info.dli_fname)
	{
		return false;
	}

	// This is for our insane sanity checks :o 
	baseAddr = reinterpret_cast<std::uintptr_t>(info.dli_fbase);
	file = reinterpret_cast<Elf32_Ehdr *>(baseAddr);

	// Check ELF magic 
	if (memcmp(ELFMAG, file->e_ident, SELFMAG) != 0)
	{
		return false;
	}

	// Check ELF version 
	if (file->e_ident[EI_VERSION] != EV_CURRENT)
	{
		return false;
	}

	// Check ELF architecture, which is 32-bit/x86 right now
	// Should change this for 64-bit if Valve gets their act together

	if (file->e_ident[EI_CLASS] != ELFCLASS32 || file->e_machine != EM_386 || file->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return false;
	}

	// For our purposes, this must be a dynamic library/shared object 
	if (file->e_type != ET_DYN)
	{
		return false;
	}

	phdrCount = file->e_phnum;
	phdr = reinterpret_cast<Elf32_Phdr *>(baseAddr + file->e_phoff);

	for (uint16_t i = 0; i < phdrCount; i++)
	{
		Elf32_Phdr &hdr = phdr[i];

		// We only really care about the segment with executable code 
		if (hdr.p_type == PT_LOAD && hdr.p_flags == (PF_X | PF_R))
		{
			// From glibc, elf/dl-load.c:
			// c->mapend = ((ph->p_vaddr + ph->p_filesz + GLRO(dl_pagesize) - 1) 
			//             & ~(GLRO(dl_pagesize) - 1));
			//
			// In glibc, the segment file size is aligned up to the nearest page size and
			// added to the virtual address of the segment. We just want the size here.

			lib.memorySize = PAGE_ALIGN_UP(hdr.p_filesz);
			break;
		}
	}
#endif

	lib.baseAddress = reinterpret_cast<void *>(baseAddr);

	return true;
}

void *CSignatureFunction::findPattern(const void *libPtr, const char *pattern, const std::size_t len)
{
	DynLibInfo lib;

	std::memset(&lib, 0, sizeof(DynLibInfo));

	if (!getLibraryInfo(libPtr, lib))
	{
		return nullptr;
	}

	char* ptr = static_cast<char*>(lib.baseAddress);
	const char* end = ptr + lib.memorySize - len;

	while (ptr < end)
	{
		bool found = true;
		for (std::size_t i = 0; i < len; i++)
		{
			if (pattern[i] != '\x2A' && pattern[i] != ptr[i])
			{
				found = false;
				break;
			}
		}

		if (found)
			return ptr;

		ptr++;
	}

	return nullptr;
}
// Sourcemod - Metamod - Allied Modders.net
void *CSignatureFunction::findSignature(const void* addrInBase, const char* signature)
{
	// First, preprocess the signature 
	unsigned char real_sig[511];

	const std::size_t real_bytes = decodeHexString(real_sig, sizeof real_sig, signature);

	if (real_bytes >= 1)
	{
		return findPattern(addrInBase, reinterpret_cast<char*>(real_sig), real_bytes);
	}

	return nullptr;
}


void CSignatureFunction::findFunc(const CRCBotKeyValueList& kv, const char* pKey, const void* pAddrBase, const char* defaultsig)
{
	const char* sig = nullptr;

	if (kv.getString(pKey, &sig) && sig)
		m_func = findSignature(pAddrBase, sig);
	else
		m_func = findSignature(pAddrBase, defaultsig);
}

CGameRulesObject::CGameRulesObject(CRCBotKeyValueList &list, void *pAddrBase)
{
#if defined(_WIN64) || defined(_WIN32)
	m_func = nullptr;
#else
	findFunc(list, "g_pGameRules", pAddrBase, "@g_pGameRules");
#endif
}

CCreateGameRulesObject::CCreateGameRulesObject(const CRCBotKeyValueList &list, const void *pAddrBase)
{
#if defined(_WIN64) || defined(_WIN32)
	findFunc(list, "create_gamerules_object_win", pAddrBase, R"(\x55\x8B\xEC\x8B\x0D\x2A\x2A\x2A\x2A\x85\xC9\x74\x07)");
#else
	m_func = nullptr;
#endif
}

void** CCreateGameRulesObject::getGameRules() const
{
	char* addr = static_cast<char*>(m_func);
	return *reinterpret_cast<void***>(addr + static_cast<std::ptrdiff_t>(rcbot_gamerules_offset.GetInt()));
}
