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

#pragma push_macro("clamp") //Fix for C++17 [APG]RoboCop[CL]
#undef clamp
#include <algorithm>
#pragma pop_macro("clamp")

#include "propvar.h"
#include "helper.h"
#include "logging.h"

CPropertyVarBase::CPropertyVarBase(): m_type()
{
	m_initialized = false;
	m_readWarned = false;
}

CPropertyVarBase::~CPropertyVarBase()
{
	m_initialized = false;
}

void CPropertyVarBase::Init(const char *propname, const PropType type, const int entity)
{
	CBaseEntity* baseentity = bot_helper->GetEntity(entity);

	if (!baseentity)
	{
		logger->Log(LogLevel::ERROR, "Initialization failed for PropertyVar \"%s\"! Entity of index %i is NULL!", propname, entity);
		return;
	}

	m_propname = std::string(propname);
	m_type = type;
	m_entity.Set(reinterpret_cast<IHandleEntity*>(baseentity));
	m_initialized = true;
	m_readWarned = false;
}

void CPropertyVarBase::Term()
{
	m_initialized = false;
}

bool CPropertyVarBase::CanRead() const
{
	if (m_initialized && entprops != nullptr && entprops->isAvailable())
		return true;

	// Warn once per variable. These accessors can run every frame, and an unthrottled
	// log line here is the sort of console volume that stalls the server log writer.
	if (!m_readWarned)
	{
		m_readWarned = true;
		logger->Log(LogLevel::WARN, "PropertyVar \"%s\" read while %s! Returning a default value; further warnings for this variable are suppressed.",
			m_propname.empty() ? "<uninitialized>" : m_propname.c_str(),
			m_initialized ? "the entity property layer is unavailable" : "not initialized");
	}

	return false;
}

int CPropertyVarInt::Get() const
{
	if (!CanRead())
		return 0;

	return entprops->GetEntProp(m_entity.GetEntryIndex(), m_type, m_propname.c_str());
}

bool CPropertyVarBool::Get() const
{
	if (!CanRead())
		return false;

	return entprops->GetEntPropBool(m_entity.GetEntryIndex(), m_type, m_propname.c_str());
}

float CPropertyVarFloat::Get() const
{
	if (!CanRead())
		return 0.0f;

	return entprops->GetEntPropFloat(m_entity.GetEntryIndex(), m_type, m_propname.c_str());
}

Vector CPropertyVarVector::Get() const
{
	if (!CanRead())
		return {0,0,0};

	return entprops->GetEntPropVector(m_entity.GetEntryIndex(), m_type, m_propname.c_str());
}

void CPropertyVarVector::Get(Vector &dest) const
{
	if (!CanRead())
	{
		dest.x = 0.0f;
		dest.y = 0.0f;
		dest.z = 0.0f;
		return;
	}

	const Vector source = entprops->GetEntPropVector(m_entity.GetEntryIndex(), m_type, m_propname.c_str());
	dest.x = source.x;
	dest.y = source.y;
	dest.z = source.z;
}