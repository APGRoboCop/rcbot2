/**
 * vim: set ts=4 :
 * =============================================================================
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 */

#include "entprops.h"
#include "bot_sm_ext.h"
#include "logging.h"
#include "helper.h"
#include "smsdk_config.h"

// From game/server/variant_t.h, same on all supported games.
class variant_t
{
public:
	union
	{
		bool bVal;
		string_t iszVal;
		int iVal;
		float flVal;
		float vecVal[3];
		color32 rgbaVal;
	};
	
	CBaseHandle eVal;
	fieldtype_t fieldType;
};

enum PropEntType
{
	PropEnt_Unknown,
	PropEnt_Handle,
	PropEnt_Entity,
	PropEnt_Edict,
	PropEnt_Variant,
};

static CBotEntProp s_entprops;
CBotEntProp *entprops = &s_entprops;

#define SET_TYPE_IF_VARIANT(type) \
	if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT) \
	{ \
		auto *pVariant = (variant_t *)((intptr_t)pEntity + offset); \
		pVariant->fieldType = type; \
	}

#define PROP_TYPE_SWITCH(type, type_name, returnval) \
	switch (pProp->GetType()) \
	{ \
	case type: \
	{ \
		if (element != 0) \
		{ \
			logger->Log(LogLevel::ERROR, "SendProp %s is not an array. Element %d is invalid.", prop, element); \
			return returnval; \
		} \
		 \
		break; \
	} \
	case DPT_Array: \
	{ \
		int elementCount = pProp->GetNumElements(); \
		int elementStride = pProp->GetElementStride(); \
		if (element < 0 || element >= elementCount) \
		{ \
			logger->Log(LogLevel::ERROR, "Element %d is out of bounds (Prop %s has %d elements).", element, prop, elementCount); \
			return returnval; \
		} \
		\
		pProp = pProp->GetArrayProp(); \
		if (!pProp) { \
			logger->Log(LogLevel::ERROR, "Error looking up ArrayProp for prop %s", prop); \
			return returnval; \
		} \
		\
		if (pProp->GetType() != (type)) \
		{ \
			logger->Log(LogLevel::ERROR, "SendProp %s type is not \"integer\" ([%d,%d] != %d)", prop, pProp->GetType(), pProp->m_nBits, type); \
			return returnval; \
		} \
		\
		offset += pProp->GetOffset() + (elementStride * element); \
		bit_count = pProp->m_nBits; \
		break; \
	} \
	case DPT_DataTable: \
	{ \
		SendTable *pTable = pProp->GetDataTable(); \
		if (!pTable) \
		{ \
			logger->Log(LogLevel::ERROR, "Error looking up DataTable for prop %s", prop); \
			return returnval; \
		} \
		\
		int elementCount = pTable->GetNumProps(); \
		if (element < 0 || element >= elementCount) \
		{ \
			logger->Log(LogLevel::ERROR, "Element %d is out of bounds (Prop %s has %d elements).", element, prop, elementCount); \
			return returnval; \
		} \
		\
		pProp = pTable->GetProp(element); \
		if (pProp->GetType() != (type)) \
		{ \
			if (pProp->GetType() != (type)) \
			{ \
				logger->Log(LogLevel::ERROR, "SendProp %s type is not " type_name " ([%d,%d] != %d)", prop, pProp->GetType(), pProp->m_nBits, type); \
				return returnval; \
			} \
		} \
		 \
		offset += pProp->GetOffset(); \
		bit_count = pProp->m_nBits; \
		break; \
	} \
	default: \
	{ \
		logger->Log(LogLevel::ERROR, "Invalid SendPropType %d for \"%s\"", pProp->GetType(), prop); \
		return returnval; \
		break; \
	} \
	}

#define CHECK_TYPE_VALID_IF_VARIANT(type, typeName, returnval) \
	if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT) \
	{ \
		auto *pVariant = (variant_t *)((intptr_t)pEntity + offset); \
		if (pVariant->fieldType != (type)) \
		{ \
			logger->Log(LogLevel::ERROR, "Variant value for %s is not a %s (%d)", prop, typeName, pVariant->fieldType); \
			return returnval; \
		} \
	}

#define CHECK_SET_PROP_DATA_OFFSET(returnval) \
	if (element < 0 || element >= td->fieldSize) \
	{ \
		logger->Log(LogLevel::ERROR, "Element %d is out of bounds (Prop %s has %d elements).", element, prop, td->fieldSize); \
		return returnval; \
	} \
	\
	offset = dinfo.actual_offset + (element * (td->fieldSizeInBytes / td->fieldSize));

#define GAMERULES_FIND_PROP_SEND(type, type_name, retval) \
	SourceMod::sm_sendprop_info_t info;\
	SendProp *pProp; \
	if (!sm_gamehelpers->FindSendPropInfo(grclassname, prop, &info)) \
	{ \
		logger->Log(LogLevel::ERROR, "Property \"%s\" not found on Game Rules", prop); \
		return retval; \
	} \
	\
	offset = info.actual_offset; \
	pProp = info.prop; \
	bit_count = pProp->m_nBits; \
	\
	switch (pProp->GetType()) \
	{ \
	case type: \
		{ \
			if (element != 0) \
			{ \
				return retval; \
			} \
			break; \
		} \
	case DPT_Array: \
		{ \
			int elementCount = pProp->GetNumElements(); \
			int elementStride = pProp->GetElementStride(); \
			if (element < 0 || element >= elementCount) \
			{ \
				return retval; \
			} \
			\
			pProp = pProp->GetArrayProp(); \
			if (!pProp) { \
				return retval; \
			} \
			\
			if (pProp->GetType() != (type)) \
			{ \
				return retval; \
			} \
			\
			offset += pProp->GetOffset() + (elementStride * element); \
			bit_count = pProp->m_nBits; \
			break; \
		} \
	case DPT_DataTable: \
		{ \
			GAMERULES_FIND_PROP_SEND_IN_SENDTABLE(info, pProp, element, type, type_name, retval); \
			\
			offset += pProp->GetOffset(); \
			bit_count = pProp->m_nBits; \
			break; \
		} \
	default: \
		{ \
			return retval; \
		} \
	} \

#define GAMERULES_FIND_PROP_SEND_IN_SENDTABLE(info, pProp, element, type, type_name, retval) \
	SendTable *pTable = (pProp)->GetDataTable(); \
	if (!pTable) \
	{ \
		return retval; \
	} \
	\
	int elementCount = pTable->GetNumProps(); \
	if ((element) < 0 || (element) >= elementCount) \
	{ \
		return retval; \
	} \
	\
	(pProp) = pTable->GetProp(element); \
	if ((pProp)->GetType() != (type)) \
	{ \
		return retval; \
	}

void CBotEntProp::Init(bool reset)
{
	if (initialized && !reset)
		return;

	SourceMod::IGameConfig *gamedata;
	char *error = nullptr;
	constexpr size_t maxlength = 0;
	grclassname = nullptr;

	if (!sm_gameconfs->LoadGameConfigFile("sdktools.games", &gamedata, error, maxlength))
	{
		logger->Log(LogLevel::ERROR, "CBotEntProp::Init -- Failed to load sdktools.game from SourceMod gamedata");
		return;
	}

	grclassname = gamedata->GetKeyValue("GameRulesProxy");

	if (!grclassname)
	{
		logger->Log(LogLevel::ERROR, "CBotEntProp::Init -- Failed to get game rules proxy classname");
	}
	else
	{
		logger->Log(LogLevel::DEBUG, "CBotEntProp::Init -- Retrieved game rules proxy classname \"%s\"", grclassname);
	}

	sm_gameconfs->CloseGameConfigFile(gamedata);
	initialized = true;
	logger->Log(LogLevel::DEBUG, "CBotEntProp::Init done");
}

/// @brief Checks if the given entity is a networked entity
/// @param pEntity Entity to check
/// @return true if the entity is networked, false otherwise
bool CBotEntProp::IsNetworkedEntity(CBaseEntity *pEntity)
{
	IServerUnknown *pUnk = reinterpret_cast<IServerUnknown*>(pEntity);
	const IServerNetworkable *pNet = pUnk->GetNetworkable();

	if (!pNet)
	{
		return false;
	}

	return true;
}

bool CBotEntProp::FindSendProp(SourceMod::sm_sendprop_info_t *info, CBaseEntity *pEntity, char *prop, int entity)
{
	IServerUnknown *pUnk = reinterpret_cast<IServerUnknown*>(pEntity);
	IServerNetworkable *pNet = pUnk->GetNetworkable();

	if (!pNet)
	{
		logger->Log(LogLevel::ERROR, "Edict %d is not networkable", entity);
		return false;
	}

	if (!sm_gamehelpers->FindSendPropInfo(pNet->GetServerClass()->GetName(), prop, info))
	{
		logger->Log(LogLevel::ERROR, "Failed to look up property \"%s\" for entity %d (%s)", prop, entity, pNet->GetServerClass()->GetName());
		return false;
	}

	return true;
}

/* Given an entity reference or index, fill out a CBaseEntity and/or edict.
   If lookup is successful, returns true and writes back the two parameters.
   If lookup fails, returns false and doesn't touch the params.  */
bool CBotEntProp::IndexToAThings(int num, CBaseEntity **pEntData, edict_t **pEdictData)
{
	CBaseEntity *pEntity = sm_gamehelpers->ReferenceToEntity(num);

	if (!pEntity)
	{
		return false;
	}

	const int index = sm_gamehelpers->ReferenceToIndex(num);
	if (index > 0 && index <= sm_players->GetMaxClients())
	{
		SourceMod::IGamePlayer *pPlayer = sm_players->GetGamePlayer(index);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return false;
		}
	}

	if (pEntData)
	{
		*pEntData = pEntity;
	}

	if (pEdictData)
	{
		edict_t *pEdict = BaseEntityToEdict(pEntity);
		if (!pEdict || pEdict->IsFree())
		{
			pEdict = nullptr;
		}

		*pEdictData = pEdict;
	}

	return true;
}

/// @brief Retrieves an integer value in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param size Number of bytes to write (valid values are 1, 2, or 4). This value is auto-detected, and the size parameter is only used as a fallback in case detection fails.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
int CBotEntProp::GetEntProp(int entity, PropType proptype, char *prop, int size, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp;
	int bit_count;
	int offset;
	bool is_unsigned = false;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return 0;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return 0;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return 0;
		}

		td = dinfo.prop;

		if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not an integer (%d)", prop, td->fieldType);
			return 0;
		}

		CHECK_SET_PROP_DATA_OFFSET(0)

		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			const auto *pVariant = reinterpret_cast<variant_t*>(reinterpret_cast<intptr_t>(pEntity) + offset);
			if ((bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0)) == 0)
			{
				logger->Log(LogLevel::ERROR, "Variant value for %s is not an integer (%d)", prop, pVariant->fieldType);
				return 0;
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return 0;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", 0)

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS \
			|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2 \
			|| SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_PVKII
			if (pProp->GetFlags() & SPROP_VARINT)
			{
				bit_count = sizeof(int) * 8;
			}
		#endif

		is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED);

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return 0;
		//break;
	}

	if (bit_count < 1)
	{
		bit_count = size * 8;
	}

	if (bit_count >= 17)
	{
		return *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
	}
	if (bit_count >= 9)
	{
		if (is_unsigned)
		{
			return *reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
		}
		return *reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
	}
	if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			return *(reinterpret_cast<uint8_t*>(pEntity) + offset);
		}
		return *reinterpret_cast<int8_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
	}
	return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(pEntity) + offset) ? 1 : 0;
}

/// @brief Retrieves an integer pointer in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param size Number of bytes to write (valid values are 1, 2, or 4). This value is auto-detected, and the size parameter is only used as a fallback in case detection fails.
/// @param element Element # (starting from 0) if property is an array.
/// @return Pointer at the given property offset.
int *CBotEntProp::GetEntPropPointer(int entity, PropType proptype, char *prop, int size, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp;
	int bit_count;
	int offset;
	bool is_unsigned = false;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return nullptr;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return nullptr;
		}

		td = dinfo.prop;

		if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not an integer (%d)", prop, td->fieldType);
			return nullptr;
		}

		CHECK_SET_PROP_DATA_OFFSET(0)

		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			const auto *pVariant = reinterpret_cast<variant_t*>(reinterpret_cast<intptr_t>(pEntity) + offset);
			if ((bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0)) == 0)
			{
				logger->Log(LogLevel::ERROR, "Variant value for %s is not an integer (%d)", prop, pVariant->fieldType);
				return nullptr;
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return nullptr;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", nullptr)

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS \
			|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2 \
			|| SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_PVKII
			if (pProp->GetFlags() & SPROP_VARINT)
			{
				bit_count = sizeof(int) * 8;
			}
		#endif

		is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED);

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return nullptr;
		//break;
	}

	if (bit_count < 1)
	{
		bit_count = size * 8;
	}

	if (bit_count >= 17)
	{
		return reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
	}
	if (bit_count >= 9)
	{
		if (is_unsigned)
		{
			return reinterpret_cast<int*>(reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset));
		}
		return reinterpret_cast<int*>(reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset));
	}
	if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			return reinterpret_cast<int*>((reinterpret_cast<uint8_t*>(pEntity) + offset));
		}
		return reinterpret_cast<int*>(reinterpret_cast<int8_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset));
	}
	return reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
}

/// @brief Retrieves a boolean value in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
bool CBotEntProp::GetEntPropBool(int entity, PropType proptype, char *prop, int element)
{
	return GetEntProp(entity, proptype, prop, 1, element) != 0;
}

/// @brief Retrieves a boolean pointer in an entity's property.
/// @attention This will return nullptr if the property bit_count is higher than 1. Use SourceMod's sm_dump_netprops to double check the property
/// you are accessing has a size of 1. Or just use GetEntPropPointer instead.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Pointer at the given property offset.
bool *CBotEntProp::GetEntPropBoolPointer(int entity, PropType proptype, char *prop, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count;
	int offset;
	bool is_unsigned = false; //Unused? [APG]RoboCop[CL]

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return nullptr;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return nullptr;
		}

		td = dinfo.prop;

		if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not an integer (%d)", prop, td->fieldType);
			return nullptr;
		}

		CHECK_SET_PROP_DATA_OFFSET(0)

		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			const auto *pVariant = reinterpret_cast<variant_t*>(reinterpret_cast<intptr_t>(pEntity) + offset);
			if ((bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0)) == 0)
			{
				logger->Log(LogLevel::ERROR, "Variant value for %s is not an integer (%d)", prop, pVariant->fieldType);
				return nullptr;
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return nullptr;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", nullptr)

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS \
			|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2 \
			|| SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_PVKII
			if (pProp->GetFlags() & SPROP_VARINT)
			{
				bit_count = sizeof(int) * 8;
			}
		#endif

		is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED); // `is_unsigned` unused? [APG]RoboCop[CL]

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return nullptr;
		//break;
	}

	if (bit_count < 1)
	{
		bit_count = 1 * 8;
	}

	if (bit_count > 1)
	{
		logger->Log(LogLevel::ERROR, "Property %s has bit_count %d > 1. Use GetEntPropPointer", prop, bit_count);
		return nullptr;
	}
	return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
}

/// @brief Sets an integer value in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param value Value to set.
/// @param size Number of bytes to write (valid values are 1, 2, or 4). This value is auto-detected, and the size parameter is only used as a fallback in case detection fails.
/// @param element Element # (starting from 0) if property is an array.
/// @return true if the value was changed, false if an error occurred
bool CBotEntProp::SetEntProp(int entity, PropType proptype, char *prop, int value, int size, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count;
	int offset;
	bool is_unsigned = false; //Unused? [APG]RoboCop[CL]

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return false;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return false;
		}

		td = dinfo.prop;

		if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not an integer (%d)", prop, td->fieldType);
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false)

		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			auto *pVariant = reinterpret_cast<variant_t*>(reinterpret_cast<intptr_t>(pEntity) + offset);
			// These are the only three int-ish types that variants support. If set to valid one that isn't
			// (32-bit) integer, leave it alone. It's probably the intended type.
			if (pVariant->fieldType != FIELD_COLOR32 && pVariant->fieldType != FIELD_BOOLEAN)
			{
				pVariant->fieldType = FIELD_INTEGER;
			}

			bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0);
		}

		SET_TYPE_IF_VARIANT(FIELD_INTEGER)

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", false)

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS \
			|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2 \
			|| SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_PVKII
			if (pProp->GetFlags() & SPROP_VARINT)
			{
				bit_count = sizeof(int) * 8;
			}
		#endif

		is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED);

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return false;
		//break;
	}

	if (bit_count < 1)
	{
		bit_count = size * 8;
	}

	if (bit_count >= 17)
	{
		*reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset) = value;
	}
	else if (bit_count >= 9)
	{
		*reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset) = static_cast<int16_t>(value);
	}
	else if (bit_count >= 2)
	{
		*reinterpret_cast<int8_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset) = static_cast<int8_t>(value);
	}
	else
	{
		*reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(pEntity) + offset) = value ? true : false;
	}
	
	if (proptype == Prop_Send && (pEdict != nullptr))
	{
		sm_gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/// @brief Sets a float value in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
float CBotEntProp::GetEntPropFloat(int entity, PropType proptype, char *prop, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	const SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count; //Unused? [APG]RoboCop[CL]
	int offset;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return 0.0f;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return 0.0f;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return 0.0f;
		}

		td = dinfo.prop;

		CHECK_SET_PROP_DATA_OFFSET(0.0f)

		CHECK_TYPE_VALID_IF_VARIANT(FIELD_FLOAT, "float", 0.0f)

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return 0.0f;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits; // `bit_count` unused? [APG]RoboCop[CL]

		PROP_TYPE_SWITCH(DPT_Float, "float", 0.0f)

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return 0.0f;
		//break;
	}

	return *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
}

/// @brief Retrieves a float pointer in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Pointer at the given property offset.
float *CBotEntProp::GetEntPropFloatPointer(int entity, PropType proptype, char *prop, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	const SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count; //Unused? [APG]RoboCop[CL]
	int offset;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return nullptr;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return nullptr;
		}

		td = dinfo.prop;

		CHECK_SET_PROP_DATA_OFFSET(nullptr)

		CHECK_TYPE_VALID_IF_VARIANT(FIELD_FLOAT, "float", nullptr)

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return nullptr;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits; // `bit_count` unused? [APG]RoboCop[CL]

		PROP_TYPE_SWITCH(DPT_Float, "float", nullptr)

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return nullptr;
		//break;
	}

	return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
}

/// @brief Sets a float value in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param value Value to set.
/// @param element Element # (starting from 0) if property is an array.
/// @return true if the value was changed, false if an error occurred
bool CBotEntProp::SetEntPropFloat(int entity, PropType proptype, char *prop, float value, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	const SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count; //Unused? [APG]RoboCop[CL]
	int offset;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return false;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return false;
		}

		td = dinfo.prop;

		if (td->fieldType != FIELD_FLOAT
			&& td->fieldType != FIELD_TIME
			&& (td->fieldType != FIELD_CUSTOM || (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not a float (%d != [%d,%d,%d]", prop, td->fieldType, FIELD_FLOAT, FIELD_TICK, FIELD_CUSTOM);
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false)

		SET_TYPE_IF_VARIANT(FIELD_FLOAT)

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits; // `bit_count` unused? [APG]RoboCop[CL]

		PROP_TYPE_SWITCH(DPT_Float, "float", false)

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return false;
		//break;
	}

	*reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(pEntity) + offset) = value;

	if (proptype == Prop_Send && (pEdict != nullptr))
	{
		sm_gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/// @brief Retrieves an entity index from an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Entity index at the given property. If there is no entity, or the entity is not valid, then -1 is returned.
int CBotEntProp::GetEntPropEnt(int entity, PropType proptype, char *prop, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	const SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count; //Unused? [APG]RoboCop[CL]
	int offset;
	PropEntType type = PropEnt_Unknown;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return -1;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return -1;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return -1;
		}

		td = dinfo.prop;

		switch (td->fieldType)
		{
		case FIELD_EHANDLE:
			type = PropEnt_Handle;
			break;
		case FIELD_CLASSPTR:
			type = PropEnt_Entity;
			break;
		case FIELD_EDICT:
			type = PropEnt_Edict;
			break;
		case FIELD_CUSTOM:
			if ((td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
			{
				type = PropEnt_Variant;
			}
			break;
		/*case FIELD_VOID:
			break;
		case FIELD_FLOAT:
			break;
		case FIELD_STRING:
			break;
		case FIELD_VECTOR:
			break;
		case FIELD_QUATERNION:
			break;
		case FIELD_INTEGER:
			break;
		case FIELD_BOOLEAN:
			break;
		case FIELD_SHORT:
			break;
		case FIELD_CHARACTER:
			break;
		case FIELD_COLOR32:
			break;
		case FIELD_EMBEDDED:
			break;
		case FIELD_POSITION_VECTOR:
			break;
		case FIELD_TIME:
			break;
		case FIELD_TICK:
			break;
		case FIELD_MODELNAME:
			break;
		case FIELD_SOUNDNAME:
			break;
		case FIELD_INPUT:
			break;
		case FIELD_FUNCTION:
			break;
		case FIELD_VMATRIX:
			break;
		case FIELD_VMATRIX_WORLDSPACE:
			break;
		case FIELD_MATRIX3X4_WORLDSPACE:
			break;
		case FIELD_INTERVAL:
			break;
		case FIELD_MODELINDEX:
			break;
		case FIELD_MATERIALINDEX:
			break;
		case FIELD_VECTOR2D:
			break;
		case FIELD_TYPECOUNT:
			break;*/
		}

		if (type == PropEnt_Unknown)
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not an entity nor edict (%d)", prop, td->fieldType);
			return -1;
		}

		CHECK_SET_PROP_DATA_OFFSET(0)

		CHECK_TYPE_VALID_IF_VARIANT(FIELD_EHANDLE, "ehandle", 0)

		break;

	case Prop_Send:
		
		type = PropEnt_Handle;

		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return -1;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits; // `bit_count` unused? [APG]RoboCop[CL]

		PROP_TYPE_SWITCH(DPT_Int, "integer", -1)

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return -1;
		//break;
	}

	switch (type)
	{
	case PropEnt_Handle:
	case PropEnt_Variant:
		{
			CBaseHandle *hndl;
			if (type == PropEnt_Handle)
			{
				hndl = reinterpret_cast<CBaseHandle*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
			}
			else // PropEnt_Variant
			{
				auto *pVariant = reinterpret_cast<variant_t*>(reinterpret_cast<intptr_t>(pEntity) + offset);
				hndl = &pVariant->eVal;
			}

			CBaseEntity *pHandleEntity = sm_gamehelpers->ReferenceToEntity(hndl->GetEntryIndex());

			if (!pHandleEntity || *hndl != reinterpret_cast<IHandleEntity *>(pHandleEntity)->GetRefEHandle())
				return -1;

			return sm_gamehelpers->EntityToBCompatRef(pHandleEntity);
		}
	case PropEnt_Entity:
		{
			CBaseEntity *pPropEntity = *reinterpret_cast<CBaseEntity**>(reinterpret_cast<uint8_t*>(pEntity) + offset);
			return sm_gamehelpers->EntityToBCompatRef(pPropEntity);
		}
	case PropEnt_Edict:
		{
			edict_t *_pEdict = *reinterpret_cast<edict_t**>(reinterpret_cast<uint8_t*>(pEntity) + offset);
			if (!_pEdict || _pEdict->IsFree())
				return -1;

			return  sm_gamehelpers->IndexOfEdict(_pEdict);
		}
	}

	return -1;
}

/// @brief Sets an entity index in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param other Entity index to set, or -1 to unset.
/// @param element Element # (starting from 0) if property is an array.
/// @return true if the value was changed, false if an error occurred
bool CBotEntProp::SetEntPropEnt(int entity, PropType proptype, char *prop, int other, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	const SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count; //Unused? [APG]RoboCop[CL]
	int offset;
	PropEntType type = PropEnt_Unknown;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return false;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return false;
		}

		td = dinfo.prop;

		switch (td->fieldType)
		{
		case FIELD_EHANDLE:
			type = PropEnt_Handle;
			break;
		case FIELD_CLASSPTR:
			type = PropEnt_Entity;
			break;
		case FIELD_EDICT:
			type = PropEnt_Edict;
			break;
		case FIELD_CUSTOM:
			if ((td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
			{
				type = PropEnt_Variant;
			}
			break;
		}

		if (type == PropEnt_Unknown)
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not an entity nor edict (%d)", prop, td->fieldType);
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false)

		CHECK_TYPE_VALID_IF_VARIANT(FIELD_EHANDLE, "ehandle", false)

		break;

	case Prop_Send:
		
		type = PropEnt_Handle;

		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits; // `bit_count` unused? [APG]RoboCop[CL]

		PROP_TYPE_SWITCH(DPT_Int, "integer", false)

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return false;
		//break;
	}

	CBaseEntity *pOther = GetEntity(other);
	if (!pOther && other != -1)
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(other), other);
		return false;
	}

	switch (type)
	{
	case PropEnt_Handle:
	case PropEnt_Variant:
		{
			CBaseHandle *hndl;
			if (type == PropEnt_Handle)
			{
				hndl = reinterpret_cast<CBaseHandle*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
			}
			else // PropEnt_Variant
			{
				auto *pVariant = reinterpret_cast<variant_t*>(reinterpret_cast<intptr_t>(pEntity) + offset);
				hndl = &pVariant->eVal;
			}

			hndl->Set(reinterpret_cast<IHandleEntity*>(pOther));

			if (proptype == Prop_Send && (pEdict != nullptr))
			{
				sm_gamehelpers->SetEdictStateChanged(pEdict, offset);
			}
		}

		break;

	case PropEnt_Entity:
		{
			*reinterpret_cast<CBaseEntity**>(reinterpret_cast<uint8_t*>(pEntity) + offset) = pOther;
			break;
		}

	case PropEnt_Edict:
		{
			edict_t *pOtherEdict = nullptr;
			if (pOther)
			{
				const IServerNetworkable *pNetworkable = reinterpret_cast<IServerUnknown*>(pOther)->GetNetworkable();
				if (!pNetworkable)
				{
					logger->Log(LogLevel::ERROR, "Entity %d does not have a valid edict", sm_gamehelpers->EntityToBCompatRef(pOther));
					return false;
				}

				pOtherEdict = pNetworkable->GetEdict();
				if (!pOtherEdict || pOtherEdict->IsFree())
				{
					logger->Log(LogLevel::ERROR, "Entity %d does not have a valid edict", sm_gamehelpers->EntityToBCompatRef(pOther));
					return false;
				}
			}

			*reinterpret_cast<edict_t**>(reinterpret_cast<uint8_t*>(pEntity) + offset) = pOtherEdict;
			break;
		}
	}

	return true;
}

/// @brief Retrieves a vector of floats from an entity, given a named network property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
Vector CBotEntProp::GetEntPropVector(int entity, PropType proptype, char *prop, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	const SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count; //Unused? [APG]RoboCop[CL]
	int offset;
	bool is_unsigned = false; //Unused? [APG]RoboCop[CL]

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return Vector(0,0,0);
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return Vector(0,0,0);
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return Vector(0,0,0);
		}

		td = dinfo.prop;

		if (td->fieldType != FIELD_VECTOR && td->fieldType != FIELD_POSITION_VECTOR)
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not a vector (%d != [%d,%d])", prop, td->fieldType, FIELD_VECTOR, FIELD_POSITION_VECTOR);
			return Vector(0,0,0);
		}

		CHECK_SET_PROP_DATA_OFFSET(Vector(0,0,0))

		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			const auto *pVariant = reinterpret_cast<variant_t*>(reinterpret_cast<intptr_t>(pEntity) + offset);
			if (pVariant->fieldType != FIELD_VECTOR && pVariant->fieldType != FIELD_POSITION_VECTOR)
			{
				logger->Log(LogLevel::ERROR, "Variant value for %s is not vector (%d)", prop, pVariant->fieldType);
				return Vector(0,0,0);
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return Vector(0,0,0);
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits; // `bit_count` unused? [APG]RoboCop[CL]

		PROP_TYPE_SWITCH(DPT_Vector, "vector", Vector(0,0,0))

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return Vector(0,0,0);
		//break;
	}

	Vector *v = reinterpret_cast<Vector*>(reinterpret_cast<uint8_t*>(pEntity) + offset);

	return *v;
}

/// @brief Retrieves a vector from an entity, given a named network property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Vector pointer from the property
Vector *CBotEntProp::GetEntPropVectorPointer(int entity, PropType proptype, char *prop, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	const SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count; //Unused? [APG]RoboCop[CL]
	int offset;
	bool is_unsigned = false; //Unused? [APG]RoboCop[CL]

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return nullptr;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return nullptr;
		}

		td = dinfo.prop;

		if (td->fieldType != FIELD_VECTOR && td->fieldType != FIELD_POSITION_VECTOR)
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not a vector (%d != [%d,%d])", prop, td->fieldType, FIELD_VECTOR, FIELD_POSITION_VECTOR);
			return nullptr;
		}

		CHECK_SET_PROP_DATA_OFFSET(nullptr)

		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			const auto *pVariant = reinterpret_cast<variant_t*>(reinterpret_cast<intptr_t>(pEntity) + offset);
			if (pVariant->fieldType != FIELD_VECTOR && pVariant->fieldType != FIELD_POSITION_VECTOR)
			{
				logger->Log(LogLevel::ERROR, "Variant value for %s is not vector (%d)", prop, pVariant->fieldType);
				return nullptr;
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return nullptr;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits; // `bit_count` unused? [APG]RoboCop[CL]

		PROP_TYPE_SWITCH(DPT_Vector, "vector", nullptr)

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return nullptr;
		//break;
	}

	Vector *v = reinterpret_cast<Vector*>(reinterpret_cast<uint8_t*>(pEntity) + offset);

	return v;
}

/// @brief Sets a vector of floats in an entity, given a named network property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param value Vector to set.
/// @param element Element # (starting from 0) if property is an array.
/// @return true if the value was changed, false if an error occurred
bool CBotEntProp::SetEntPropVector(int entity, PropType proptype, char *prop, Vector value, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	const SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count; //Unused? [APG]RoboCop[CL]
	int offset;
	bool is_unsigned = false; //Unused? [APG]RoboCop[CL]

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return false;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return false;
		}

		td = dinfo.prop;

		if (td->fieldType != FIELD_VECTOR && td->fieldType != FIELD_POSITION_VECTOR)
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not a vector (%d != [%d,%d])", prop, td->fieldType, FIELD_VECTOR, FIELD_POSITION_VECTOR);
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false)

		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			auto *pVariant = reinterpret_cast<variant_t*>(reinterpret_cast<intptr_t>(pEntity) + offset);
			// Both of these are supported and we don't know which is intended. But, if it's already
			// a pos vector, we probably want to keep that.
			if (pVariant->fieldType != FIELD_POSITION_VECTOR)
			{
				pVariant->fieldType = FIELD_VECTOR;
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits; //Unused? [APG]RoboCop[CL]

		PROP_TYPE_SWITCH(DPT_Vector, "vector", false)

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return false;
		//break;
	}

	Vector *v = reinterpret_cast<Vector*>(reinterpret_cast<uint8_t*>(pEntity) + offset);

	*v = value;

	if (proptype == Prop_Send && (pEdict != nullptr))
	{
		sm_gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/// @brief Gets a network property as a string.
/// @param entity Edict index.
/// @param proptype Property type.
/// @param prop Property to use.
/// @param maxlen
/// @param len 
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
char *CBotEntProp::GetEntPropString(int entity, PropType proptype, char *prop, int maxlen, int *len, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	const SendProp *pProp = nullptr; //Unused? [APG]RoboCop[CL]
	int bit_count; //Unused? [APG]RoboCop[CL]
	int offset;
	const char *src = nullptr; //Unused? [APG]RoboCop[CL]
	char *dest = nullptr;
	bool bIsStringIndex = false; //Unused? [APG]RoboCop[CL]

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == nullptr)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return nullptr;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return nullptr;
		}

		td = dinfo.prop;

		if ((td->fieldType != FIELD_CHARACTER
			&& td->fieldType != FIELD_STRING
			&& td->fieldType != FIELD_MODELNAME 
			&& td->fieldType != FIELD_SOUNDNAME)
			|| (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not a string (%d != %d)", prop, td->fieldType, FIELD_CHARACTER);
			return nullptr;
		}

		bIsStringIndex = (td->fieldType != FIELD_CHARACTER);

		if (element != 0)
		{
			if (bIsStringIndex)
			{
				if (element < 0 || element >= td->fieldSize)
				{
					logger->Log(LogLevel::ERROR, "Element %d is out of bounds (Prop %s has %d elements).", element, prop, td->fieldSize);
					return nullptr;
				}
			}
			else
			{
				logger->Log(LogLevel::ERROR, "Prop %s is not an array. Element %d is invalid.", prop, element);
				return nullptr;
			}
		}

		offset = dinfo.actual_offset;

		if (bIsStringIndex)
		{
			offset += (element * (td->fieldSizeInBytes / td->fieldSize));

			const string_t idx = *reinterpret_cast<string_t*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
			src = (idx == NULL_STRING) ? "" : STRING(idx);
		}
		else
		{
			src = reinterpret_cast<char*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return nullptr;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits; // `bit_count` unused? [APG]RoboCop[CL]

		PROP_TYPE_SWITCH(DPT_String, "string", nullptr)

		if (pProp->GetProxyFn())
		{
			DVariant var;
			pProp->GetProxyFn()(pProp, pEntity, reinterpret_cast<const void*>(reinterpret_cast<intptr_t>(pEntity) + offset), &var, element, entity);
			src = var.m_pString; // hack because SDK 2013 declares this as const char*
		}
		else
		{
			src = *reinterpret_cast<char**>(reinterpret_cast<uint8_t*>(pEntity) + offset);
		}

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return nullptr;
		//break;
	}

	const size_t length = ke::SafeStrcpy(dest, maxlen, src);
	*len = length;

	return dest;
}


/// @brief Sets a network property as a string.
/// @warning This is not implemented yet!
bool CBotEntProp::SetEntPropString(int entity, PropType proptype, char *prop, char *value, int element)
{
	logger->Log(LogLevel::ERROR, "SetEntPropString is not supported for now");
	return false;

#if 0 // Not supported for now

	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr;
	int bit_count;
	int offset;
	int maxlen;
	bool bIsStringIndex = false;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = sm_gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return false;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!sm_gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = sm_gamehelpers->GetEntityClassname(pEntity);
			logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return false;
		}

		td = dinfo.prop;

		if ((td->fieldType != FIELD_CHARACTER
			&& td->fieldType != FIELD_STRING
			&& td->fieldType != FIELD_MODELNAME 
			&& td->fieldType != FIELD_SOUNDNAME)
			|| (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
		{
			logger->Log(LogLevel::ERROR, "Data field %s is not a string (%d != %d)", prop, td->fieldType, FIELD_CHARACTER);
			return false;
		}

		bIsStringIndex = (td->fieldType != FIELD_CHARACTER);

		if (element != 0)
		{
			if (bIsStringIndex)
			{
				if (element < 0 || element >= td->fieldSize)
				{
					logger->Log(LogLevel::ERROR, "Element %d is out of bounds (Prop %s has %d elements).", element, prop, td->fieldSize);
					return false;
				}
			}
			else
			{
				logger->Log(LogLevel::ERROR, "Prop %s is not an array. Element %d is invalid.", prop, element);
				return false;
			}
		}

		offset = dinfo.actual_offset;

		if (bIsStringIndex)
		{
			offset += (element * (td->fieldSizeInBytes / td->fieldSize));
		}
		else
		{
			maxlen = td->fieldSize;
		}

		SET_TYPE_IF_VARIANT(FIELD_STRING);

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_String, "string", false);

		bIsStringIndex = false;
		if (pProp->GetProxyFn())
		{
			DVariant var;
			pProp->GetProxyFn()(pProp, pEntity, (const void *) ((intptr_t) pEntity + offset), &var, element, entity);
			if (var.m_pString == ((string_t *) ((intptr_t) pEntity + offset))->ToCStr())
			{
				bIsStringIndex = true;
			}
		}

		// Only used if not string index.
		maxlen = DT_MAX_STRING_BUFFERSIZE;

		break;
	
	default:
		logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return false;
		break;
	}

	size_t len;

	if (bIsStringIndex)
	{
		*(string_t *) ((intptr_t) pEntity + offset) = g_HL2.AllocPooledString(value);
		len = strlen(value);
	}
	else
	{
		char *dest = (char *) ((uint8_t *) pEntity + offset);
		len = ke::SafeStrcpy(dest, maxlen, value);
	}

	if (proptype == Prop_Send && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return true;

#endif
}

/// @brief Peeks into an entity's object data and retrieves the integer value at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param size Number of bytes to read (valid values are 1, 2, or 4).
/// @return Value at the given memory location.
int CBotEntProp::GetEntData(int entity, int offset, int size)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return 0;
	}

	if (offset <= 0 || offset > 32768)
	{
		logger->Log(LogLevel::ERROR, "Offset %d is invalid", offset);
		return 0;
	}

	switch (size)
	{
	case 4:
		return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
	case 2:
		return *reinterpret_cast<short*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
	case 1:
		return *(reinterpret_cast<uint8_t*>(pEntity) + offset);
	default:
		logger->Log(LogLevel::ERROR, "Integer size %d is invalid", size);
		return 0;
	}
}

/// @brief Peeks into an entity's object data and sets the integer value at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param value Value to set.
/// @param size Number of bytes to write (valid values are 1, 2, or 4).
/// @param changeState If true, change will be sent over the network.
/// @return true on success, false on failure
bool CBotEntProp::SetEntData(int entity, int offset, int value, int size, bool changeState)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		logger->Log(LogLevel::ERROR, "Offset %d is invalid", offset);
		return false;
	}

	if (changeState && (pEdict != nullptr))
	{
		sm_gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	switch (size)
	{
	case 4:
		{
			*reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(pEntity) + offset) = value;
			break;
		}
	case 2:
		{
			*reinterpret_cast<short*>(reinterpret_cast<uint8_t*>(pEntity) + offset) = value;
			break;
		}
	case 1:
		{
			*(reinterpret_cast<uint8_t*>(pEntity) + offset) = value;
			break;
		}
	default:
		logger->Log(LogLevel::ERROR, "Integer size %d is invalid", size);
		return false;
	}

	return true;
}

/// @brief Peeks into an entity's object data and retrieves the float value at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @return Value at the given memory location.
float CBotEntProp::GetEntDataFloat(int entity, int offset)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return 0.0f;
	}

	if (offset <= 0 || offset > 32768)
	{
		logger->Log(LogLevel::ERROR, "Offset %d is invalid", offset);
		return 0.0f;
	}

	return *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
}

/// @brief Peeks into an entity's object data and sets the float value at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param value Value to set.
/// @param changeState If true, change will be sent over the network.
/// @return true on success, false on failure
bool CBotEntProp::SetEntDataFloat(int entity, int offset, float value, bool changeState)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		logger->Log(LogLevel::ERROR, "Offset %d is invalid", offset);
		return false;
	}

	*reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(pEntity) + offset) = value;

	if (changeState && (pEdict != nullptr))
	{
		sm_gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/// @brief Peeks into an entity's object data and retrieves the entity index at the given offset.
/// Note: This will only work on offsets that are stored as "entity
/// handles" (which usually looks like m_h* in properties).  These
/// are not SourceMod Handles, but internal Source structures.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @return Entity index at the given location. If there is no entity, or the stored entity is invalid, then -1 is returned.
int CBotEntProp::GetEntDataEnt(int entity, int offset)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return -1;
	}

	if (offset <= 0 || offset > 32768)
	{
		logger->Log(LogLevel::ERROR, "Offset %d is invalid", offset);
		return -1;
	}

	const CBaseHandle &hndl = *reinterpret_cast<CBaseHandle*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
	CBaseEntity *pHandleEntity = sm_gamehelpers->ReferenceToEntity(hndl.GetEntryIndex());

	if (!pHandleEntity || hndl != reinterpret_cast<IHandleEntity *>(pHandleEntity)->GetRefEHandle())
		return -1;

	return sm_gamehelpers->EntityToBCompatRef(pHandleEntity);
}

/// @brief Peeks into an entity's object data and sets the entity index at the given offset.
/// Note: This will only work on offsets that are stored as "entity
/// handles" (which usually looks like m_h* in properties).  These
/// are not SourceMod Handles, but internal Source structures.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param value Entity index to set, or -1 to clear.
/// @param changeState If true, change will be sent over the network.
/// @return true on success, false on failure
bool CBotEntProp::SetEntDataEnt(int entity, int offset, int value, bool changeState)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		logger->Log(LogLevel::ERROR, "Offset %d is invalid", offset);
		return false;
	}

	CBaseHandle &hndl = *reinterpret_cast<CBaseHandle*>(reinterpret_cast<uint8_t*>(pEntity) + offset);

	if (static_cast<unsigned>(value) == INVALID_EHANDLE_INDEX)
	{
		hndl.Set(nullptr);
	}
	else
	{
		CBaseEntity *pOther = GetEntity(value);

		if (!pOther)
		{
			logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(value), value);
			return false;
		}

		const IHandleEntity *pHandleEnt = reinterpret_cast<IHandleEntity*>(pOther);
		hndl.Set(pHandleEnt);
	}

	if (changeState && (pEdict != nullptr))
	{
		sm_gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/// @brief Peeks into an entity's object data and retrieves the vector at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @return Vector value at the given memory location.
Vector CBotEntProp::GetEntDataVector(int entity, int offset)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return Vector(0,0,0);
	}

	if (offset <= 0 || offset > 32768)
	{
		logger->Log(LogLevel::ERROR, "Offset %d is invalid", offset);
		return Vector(0,0,0);
	}

	Vector *v = reinterpret_cast<Vector*>(reinterpret_cast<uint8_t*>(pEntity) + offset);

	return *v;
}

/// @brief Peeks into an entity's object data and sets the vector at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param value Vector to set.
/// @param changeState If true, change will be sent over the network.
/// @return true on success, false on failure
bool CBotEntProp::SetEntDataVector(int entity, int offset, Vector value, bool changeState)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		logger->Log(LogLevel::ERROR, "Offset %d is invalid", offset);
		return false;
	}

	Vector *v = reinterpret_cast<Vector*>(reinterpret_cast<uint8_t*>(pEntity) + offset);

	*v = value;

	if (changeState && (pEdict != nullptr))
	{
		sm_gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/// @brief Peeks into an entity's object data and retrieves the string at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param maxlen Maximum length of output string buffer.
/// @param len Number of non-null bytes written.
/// @return String pointer at the given memory location.
char *CBotEntProp::GetEntDataString(int entity, int offset, int maxlen, int *len)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return nullptr;
	}

	if (offset <= 0 || offset > 32768)
	{
		logger->Log(LogLevel::ERROR, "Offset %d is invalid", offset);
		return nullptr;
	}

	if (maxlen <= 0)
	{
		logger->Log(LogLevel::ERROR, "String length should be at least 1, is %d", maxlen);
		return nullptr;
	}

	const char *src = reinterpret_cast<char*>(reinterpret_cast<uint8_t*>(pEntity) + offset);
	char *dest = nullptr;
	const size_t length = ke::SafeStrcpy(dest, maxlen, src);
	*len = length;
	return dest;
}

/// @brief 
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param value String to set.
/// @param maxlen Maximum length of output string buffer.
/// @param changeState If true, change will be sent over the network.
/// @return true on success, false on failure
bool CBotEntProp::SetEntDataString(int entity, int offset, char *value, int maxlen, bool changeState)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", sm_gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		logger->Log(LogLevel::ERROR, "Offset %d is invalid", offset);
		return false;
	}

	const char *src = nullptr;
	char *dest = reinterpret_cast<char*>(reinterpret_cast<uint8_t*>(pEntity) + offset);

	ke::SafeStrcpy(dest, maxlen, src);

	if (changeState && (pEdict != nullptr))
	{
		sm_gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

CBaseEntity *CBotEntProp::GetGameRulesProxyEntity()
{
	static int proxyEntRef = -1;
	CBaseEntity *pProxy;
	if (proxyEntRef == -1 || (pProxy = sm_gamehelpers->ReferenceToEntity(proxyEntRef)) == nullptr)
	{
		pProxy = GetEntity(bot_helper->FindEntityByNetClass(sm_players->GetMaxClients(), grclassname));
		if (pProxy)
			proxyEntRef = sm_gamehelpers->EntityToReference(pProxy);
	}
	
	return pProxy;
}

/// @brief Retrieves an integer value from a property of the gamerules entity.
/// @param prop Property name.
/// @param size Number of bytes to read (valid values are 1, 2, or 4). This value is auto-detected, and the size parameter is only used as a fallback in case detection fails.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
int CBotEntProp::GameRules_GetProp(char *prop, int size, int element) const
{
	int offset;
	int bit_count;
	bool is_unsigned = false; //Unused? [APG]RoboCop[CL]
	void *pGameRules = sm_sdktools->GetGameRules();

	if (!pGameRules || !grclassname || !strcmp(grclassname, ""))
	{
		logger->Log(LogLevel::ERROR, "Gamerules lookup failed");
		return -1;
	}

	int elementCount = 1; //Unused? [APG]RoboCop[CL]
	GAMERULES_FIND_PROP_SEND(DPT_Int, "integer", -1)
	is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED);

	// This isn't in CS:S yet, but will be, doesn't hurt to add now, and will save us a build later
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_PVKII
	if (pProp->GetFlags() & SPROP_VARINT)
	{
		bit_count = sizeof(int) * 8;
	}
#endif

	if (bit_count < 1)
	{
		bit_count = size * 8;
	}

	if (bit_count >= 17)
	{
		return *reinterpret_cast<int32_t*>(reinterpret_cast<intptr_t>(pGameRules) + offset);
	}
	if (bit_count >= 9)
	{
		if (is_unsigned)
		{
			return *reinterpret_cast<uint16_t*>(reinterpret_cast<intptr_t>(pGameRules) + offset);
		}
		return *reinterpret_cast<int16_t*>(reinterpret_cast<intptr_t>(pGameRules) + offset);
	}
	if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			return *reinterpret_cast<uint8_t*>(reinterpret_cast<intptr_t>(pGameRules) + offset);
		}
		return *reinterpret_cast<int8_t*>(reinterpret_cast<intptr_t>(pGameRules) + offset);
	}
	return *reinterpret_cast<bool*>(reinterpret_cast<intptr_t>(pGameRules) + offset) ? 1 : 0;

	//return -1;
}

/// @brief Retrieves a float value from a property of the gamerules entity.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
float CBotEntProp::GameRules_GetPropFloat(char *prop, int element) const
{
	int offset;
	int bit_count; //Unused? [APG]RoboCop[CL]
	void *pGameRules = sm_sdktools->GetGameRules();

	if (!pGameRules || !grclassname || !strcmp(grclassname, ""))
	{
		logger->Log(LogLevel::ERROR, "Gamerules lookup failed");
		return 0.0f;
	}

	int elementCount = 1; //Unused? [APG]RoboCop[CL]
	GAMERULES_FIND_PROP_SEND(DPT_Float, "float", 0.0f)

	return *reinterpret_cast<float*>(reinterpret_cast<intptr_t>(pGameRules) + offset);
}

/// @brief Retrieves a entity index from a property of the gamerules entity.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Entity index at the given property. If there is no entity, or the entity is not valid, then -1 is returned.
int CBotEntProp::GameRules_GetPropEnt(char *prop, int element) const
{
	int offset;
	int bit_count; //Unused? [APG]RoboCop[CL]
	void *pGameRules = sm_sdktools->GetGameRules();

	if (!pGameRules || !grclassname || !strcmp(grclassname, ""))
	{
		logger->Log(LogLevel::ERROR, "Gamerules lookup failed");
		return 0.0f;
	}

	int elementCount = 1; //Unused? [APG]RoboCop[CL]
	GAMERULES_FIND_PROP_SEND(DPT_Int, "Integer", 0.0f)

	const CBaseHandle &hndl = *reinterpret_cast<CBaseHandle*>(reinterpret_cast<intptr_t>(pGameRules) + offset);
	CBaseEntity *pEntity = sm_gamehelpers->ReferenceToEntity(hndl.GetEntryIndex());

	if (!pEntity || reinterpret_cast<IServerEntity*>(pEntity)->GetRefEHandle() != hndl)
	{
		return -1;
	}

	return sm_gamehelpers->EntityToBCompatRef(pEntity);
}

/// @brief Retrieves a vector from the gamerules entity, given a named network property.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
Vector CBotEntProp::GameRules_GetPropVector(char *prop, int element) const
{
	int offset;
	int bit_count; //Unused? [APG]RoboCop[CL]
	void *pGameRules = sm_sdktools->GetGameRules();

	if (!pGameRules || !grclassname || !strcmp(grclassname, ""))
	{
		logger->Log(LogLevel::ERROR, "Gamerules lookup failed");
		return Vector(0,0,0);
	}

	int elementCount = 1; //Unused? [APG]RoboCop[CL]
	GAMERULES_FIND_PROP_SEND(DPT_Vector, "vector", Vector(0,0,0))

	return *reinterpret_cast<Vector*>(reinterpret_cast<intptr_t>(pGameRules) + offset);
}

/// @brief Gets a gamerules property as a string.
/// @param prop Property to use.
/// @param len Number of non-null bytes written.
/// @param maxlen Maximum length of output string buffer.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
char *CBotEntProp::GameRules_GetPropString(char *prop, int *len, int maxlen, int element) const
{
	int offset;
	int bit_count; //Unused? [APG]RoboCop[CL]
	void *pGameRules = sm_sdktools->GetGameRules();

	if (!pGameRules || !grclassname || !strcmp(grclassname, ""))
	{
		logger->Log(LogLevel::ERROR, "Gamerules lookup failed");
		return nullptr;
	}

	int elementCount = 1; //Unused? [APG]RoboCop[CL]
	GAMERULES_FIND_PROP_SEND(DPT_String, "string", nullptr)

	const char *src;
	char *dest = nullptr;
	if (pProp->GetProxyFn())
	{
		DVariant var;
		pProp->GetProxyFn()(pProp, pGameRules, reinterpret_cast<const void*>(reinterpret_cast<intptr_t>(pGameRules) + offset), &var, element, 0 /* TODO */);
		src = var.m_pString;
	}
	else
	{
		src = *reinterpret_cast<char**>(static_cast<uint8_t*>(pGameRules) + offset);
	}

	if (src)
	{
		const size_t length = ke::SafeStrcpy(dest, maxlen, src);
		*len = length;
	}

	return dest;
}

RoundState CBotEntProp::GameRules_GetRoundState() const
{
	char roundState[] = "m_iRoundState";
	return static_cast<RoundState>(GameRules_GetProp(roundState));
}