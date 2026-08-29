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

#ifndef _PROPERTY_VARIABLE_H_
#define _PROPERTY_VARIABLE_H_

#include <string>
#include "entprops.h"
#include "vector.h"

/// @brief Base class for easy access to entity network propteries and datamaps.
class CPropertyVarBase
{
public:
	CPropertyVarBase();
	virtual ~CPropertyVarBase();

	/// @brief Initializes the property variable
	/// @param propname Property name. Get a netprops and datamaps dump for a list of available property names.
	/// @param type Property type. Prop_Send for network propertys and Prop_Data for datamaps.
	/// @param entity Entity index to read the property from.
	virtual void Init(const char* propname, PropType type, int entity);
	/// @brief Checks if the property is initialized with a property name, type and entity index.
	bool IsInitialized() const { return m_initialized; }
	/// @brief Marks this as not initialized.
	virtual void Term();
protected:
	/// @brief True when a Get() call is safe: the var was initialized and the entprops
	/// helpers are live. Get() returns a zero value instead of reading through an
	/// invalid handle when this is false, warning once via m_readWarned.
	bool CanRead() const;

	std::string m_propname;
	PropType m_type;
	CBaseHandle m_entity;
	bool m_initialized;
	// Latches on the first failed CanRead() so these accessors, which may run every
	// frame, warn once instead of flooding the console. Cleared by a successful Init().
	mutable bool m_readWarned;
};

class CPropertyVarInt : public CPropertyVarBase
{
public:
	int Get() const;
};

class CPropertyVarBool : public CPropertyVarBase
{
public:
	bool Get() const;
};

class CPropertyVarFloat : public CPropertyVarBase
{
public:
	float Get() const;
};

class CPropertyVarVector : public CPropertyVarBase
{
public:
	Vector Get() const;
	void Get(Vector &dest) const;
};

#endif