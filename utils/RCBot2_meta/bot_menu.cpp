// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/*
 *    part of https://rcbot2.svn.sourceforge.net/svnroot/rcbot2
 *
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
#include "engine_wrappers.h"
#include "ndebugoverlay.h"

#include "bot_cvars.h"
#include "bot_waypoint.h"
#include "bot_menu.h"
#include "bot_wpt_color.h"
#include "bot_globals.h"
#include "bot_client.h"

#include <string>
#include <vector>

extern IVDebugOverlay* debugoverlay;

CBotMenu* CBotMenuList::m_MenuList[BOT_MENU_MAX];

void CWaypointFlagMenuItem::activate(CClient* pClient)
{
	const int iWpt = pClient->currentWaypoint();
	CWaypoint* pWpt = CWaypoints::getWaypoint(iWpt);
	const CWaypointType* type = CWaypointTypes::getTypeByIndex(m_iFlag);

	if (pWpt)
	{
		if (pWpt->hasFlag(type->getBits()))
			pWpt->removeFlag(type->getBits());
		else
			pWpt->addFlag(type->getBits());
	}
}

const char* CWaypointFlagMenu::getCaption(CClient* pClient, WptColor& color)
{
	pClient->updateCurrentWaypoint();

	const int iWpt = pClient->currentWaypoint();

	if (const CWaypoint* pWpt = CWaypoints::getWaypoint(iWpt))
	{
		color = CWaypointTypes::getColour(pWpt->getFlags());
		snprintf(m_szCaption, sizeof(m_szCaption), "Waypoint Flags ID = [%d]", iWpt);
	}
	else
	{
		color = WptColor::white;
		snprintf(m_szCaption, sizeof(m_szCaption), "No Waypoint");
	}

	return m_szCaption;
}

const char* CWaypointFlagMenuItem::getCaption(CClient* pClient, WptColor& color)
{
	pClient->updateCurrentWaypoint();

	const int iWpt = pClient->currentWaypoint();
	const CWaypoint* pWpt = CWaypoints::getWaypoint(iWpt);

	const CWaypointType* type = CWaypointTypes::getTypeByIndex(m_iFlag);

	color = type->getColour();

	snprintf(m_szCaption, sizeof(m_szCaption), "[%s] %s", pWpt != nullptr ?
		(pWpt->hasFlag(type->getBits()) ? "x" : " ") : "No Waypoint", type->getName());

	return m_szCaption;
}

CWaypointFlagMenu::CWaypointFlagMenu(CBotMenu* pPrev)
{
	const int iMod = CBotGlobals::getCurrentMod()->getModId();
	// check the number of waypoint types available
	// caption
	// 1.
	// 2.
	// 3.
	// 4.
	// 5.
	// 6.
	// 7.
	// 8. More...
	// 9. Go Back

	const std::size_t iNumTypes = CWaypointTypes::getNumTypes();

	std::size_t iNumAdded = 0;

	CBotMenu* pCurrent = this;
	CBotMenu* pParent = pPrev;

	for (std::size_t i = 0; i < iNumTypes; i++)
	{
		if (!CWaypointTypes::getTypeByIndex(i)->forMod(iMod))
			continue;

		pCurrent->addMenuItem(new CWaypointFlagMenuItem(static_cast<int>(i)));
		iNumAdded++;

		if (iNumAdded > 7 || i == iNumTypes - 1)
		{
			CBotMenuItem* back = new CBotGotoMenuItem("Back...", pParent);

			pParent = pCurrent;

			if (iNumAdded > 7 && i < iNumTypes - 1)
			{
				pCurrent = new CBotMenu();
				pCurrent->setCaption("Waypoint Flags (More)");
				pParent->addMenuItem(new CBotGotoMenuItem("More...", pCurrent));
			}

			pParent->addMenuItem(back);

			//make a new menu

			iNumAdded = 0; // reset
		}
	}
}

void CBotMenuList::setupMenus()
{
	m_MenuList[BOT_MENU_WPT] = new CWaypointMenu();
}

const char* CWaypointRadiusMenu::getCaption(CClient* pClient, WptColor& color)
{
	const int iWpt = pClient->currentWaypoint();
	const CWaypoint* pWpt = CWaypoints::getWaypoint(iWpt);
	float fRadius = 0.0f;

	if (pWpt)
	{
		fRadius = pWpt->getRadius();
	}

	snprintf(m_szCaption, sizeof(m_szCaption), "Waypoint Radius (%0.1f)", fRadius);
	color = WptColor::white;

	return m_szCaption;
}

const char* CWaypointAreaMenu::getCaption(CClient* pClient, WptColor& color)
{
	const int iWpt = pClient->currentWaypoint();
	const CWaypoint* pWpt = CWaypoints::getWaypoint(iWpt);
	int iArea = 0;

	if (pWpt)
	{
		iArea = pWpt->getArea();
	}

	snprintf(m_szCaption, sizeof(m_szCaption), "Waypoint Area (%d)", iArea);
	color = WptColor::white;

	return m_szCaption;
}

const char* CWaypointMenu::getCaption(CClient* pClient, WptColor& color)
{
	const int iWpt = pClient->currentWaypoint();

	if (iWpt == -1)
		snprintf(m_szCaption, sizeof(m_szCaption), "Waypoint Menu - No waypoint - Walk towards a waypoint");
	else
		snprintf(m_szCaption, sizeof(m_szCaption), "Waypoint Menu [%d]", iWpt);

	color = WptColor::white;

	return m_szCaption;
}

const char* CWaypointYawMenuItem::getCaption(CClient* pClient, WptColor& color)
{
	if (const CWaypoint* pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint()))
		snprintf(m_szCaption, sizeof(m_szCaption), "Yaw = %d degrees (press to update)", static_cast<int>(pWpt->getAimYaw()));
	else
		snprintf(m_szCaption, sizeof(m_szCaption), "No Waypoint");

	return m_szCaption;
}

void CWaypointYawMenuItem::activate(CClient* pClient)
{
	if (CWaypoint* pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint()))
		pWpt->setAim(CBotGlobals::playerAngles(pClient->getPlayer()).y);
}

void CWaypointAreaIncrease::activate(CClient* pClient)
{
	const int iWpt = pClient->currentWaypoint();

	if (CWaypoint* pWpt = CWaypoints::getWaypoint(iWpt))
	{
		pWpt->setArea(pWpt->getArea() + 1);
	}
}

void CWaypointAreaDecrease::activate(CClient* pClient)
{
	const int iWpt = pClient->currentWaypoint();

	if (CWaypoint* pWpt = CWaypoints::getWaypoint(iWpt))
	{
		pWpt->setArea(pWpt->getArea() - 1);
	}
}

void CWaypointRadiusIncrease::activate(CClient* pClient)
{
	const int iWpt = pClient->currentWaypoint();

	if (CWaypoint* pWpt = CWaypoints::getWaypoint(iWpt))
	{
		const float fRadius = pWpt->getRadius();

		if (fRadius < 200.0f)
			pWpt->setRadius(fRadius + 32.0f);
		else
			pWpt->setRadius(200.0f);
	}
}

void CWaypointRadiusDecrease::activate(CClient* pClient)
{
	const int iWpt = pClient->currentWaypoint();

	if (CWaypoint* pWpt = CWaypoints::getWaypoint(iWpt))
	{
		if (const float fRadius = pWpt->getRadius(); fRadius > 32.0f)
			pWpt->setRadius(fRadius - 32.0f);
		else
			pWpt->setRadius(0.0f);
	}
}

const char* CWaypointCutMenuItem::getCaption(CClient* pClient, WptColor& color)
{
	snprintf(m_szCaption, sizeof(m_szCaption), "Cut Waypoint");
	color = WptColor::white;

	return m_szCaption;
}

void CWaypointCutMenuItem::activate(CClient* pClient)
{
	pClient->updateCurrentWaypoint();

	if (const CWaypoint* pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint()))
	{
		pClient->setWaypointCut(pWpt);
		CWaypoints::deleteWaypoint(pClient->currentWaypoint());
	}
}

const char* CWaypointCopyMenuItem::getCaption(CClient* pClient, WptColor& color)
{
	snprintf(m_szCaption, sizeof(m_szCaption), "Copy Waypoint");
	color = WptColor::white;

	return m_szCaption;
}

void CWaypointCopyMenuItem::activate(CClient* pClient)
{
	pClient->updateCurrentWaypoint();

	if (const CWaypoint* pWpt = CWaypoints::getWaypoint(pClient->currentWaypoint()))
	{
		pClient->setWaypointCopy(pWpt);
	}
}

const char* CWaypointPasteMenuItem::getCaption(CClient* pClient, WptColor& color)
{
	snprintf(m_szCaption, sizeof(m_szCaption), "Paste Waypoint");
	color = WptColor::white;

	return m_szCaption;
}

void CWaypointPasteMenuItem::activate(CClient* pClient)
{
	CWaypoints::addWaypoint(pClient, nullptr, nullptr, nullptr, nullptr, true);
}

void CBotMenu::render(CClient* pClient)
{
	WptColor color;
	Vector vForward;
	IPlayerInfo* pPlayerInfo = playerinfomanager->GetPlayerInfo(pClient->getPlayer());
	const CBotCmd lastCmd = pPlayerInfo->GetLastUserCommand();
	const float fUpdateTime = rcbot_menu_update_time1.GetFloat();

	const QAngle angles = lastCmd.viewangles;

	Vector vOrigin = pPlayerInfo->GetAbsOrigin();

	AngleVectors(angles, &vForward);

	vForward = vForward / vForward.Length();

	const Vector vRight = vForward.Cross(Vector(0, 0, 1));

	vOrigin = vOrigin + vForward * 100 - vRight * 100;
	vOrigin.z += 72.0f;

	const char* pszCaption = getCaption(pClient, color);

	//TODO: to allow waypoint menu work for the newer SDK2013 for CSS, DODS, HL2DM and TF2 [APG]RoboCop[CL]
//#if SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_HL2DM 
//	debugoverlay->AddScreenTextOverlay(0.135f, 0.4f, 0, fUpdateTime, color.r, color.g, color.b, color.a, pszCaption);
//	debugoverlay->AddScreenTextOverlay(0.135f, 0.4f, 1, fUpdateTime, color.r, color.g, color.b, color.a, "----------------");
//#else
	debugoverlay->AddTextOverlayRGB(vOrigin, 0, fUpdateTime, color.r, color.g, color.b, color.a, pszCaption);
	debugoverlay->AddTextOverlayRGB(vOrigin, 1, fUpdateTime, color.r, color.g, color.b, color.a, "----------------");
//#endif
	/*
		Vector screen;
		Vector point = Vector(0,0,0);

		debugoverlay->ScreenPosition(0.5f, 0.5f, screen);
		debugoverlay->ScreenPosition(point,screen);*/

	for (int i = 0; i < static_cast<int>(m_MenuItems.size()); i++)
	{
		CBotMenuItem* item = m_MenuItems[i];

		pszCaption = item->getCaption(pClient, color);

		debugoverlay->AddTextOverlayRGB(vOrigin, i + 2, fUpdateTime, color.r, color.g, color.b, color.a, "%d. %s", i == 9 ? 0 : i + 1, pszCaption);
	}
}

const char* CBotMenuItem::getCaption(CClient* pClient, WptColor& color)
{
	color.r = 255;
	color.g = 255;
	color.b = 255;
	color.a = 255;

	return m_szCaption;
}

void CBotMenuList::render(CClient* pClient) // render
{
	CBotMenu* pMenu = pClient->getCurrentMenu();

	pMenu->render(pClient);
	//m_MenuList[iMenu]->render(pClient);
}

void CBotMenuList::selectedMenu(CClient* pClient, const unsigned iMenu)
{
	const CBotMenu* pMenu = pClient->getCurrentMenu();

	pMenu->selectedMenu(pClient, iMenu);
}

void CBotMenu::activate(CClient* pClient)
{
	pClient->setCurrentMenu(this);
}

//TODO: Experimental [APG]RoboCop[CL]
Color CBotMenu::getColor(CClient* pClient)
{
	return Color();
}

void CBotMenu::selectedMenu(CClient* pClient, const unsigned iMenu) const
{
	if (iMenu < m_MenuItems.size())
		m_MenuItems[iMenu]->activate(pClient);
}

CWaypointFlagShowMenu::CWaypointFlagShowMenu(CBotMenu* pParent)
{
	const int iMod = CBotGlobals::getCurrentMod()->getModId();
	// check the number of waypoint types available
	// caption
	// 1.
	// 2.
	// 3.
	// 4.
	// 5.
	// 6.
	// 7.
	// 8. More...
	// 9. Go Back

	const std::size_t iNumTypes = CWaypointTypes::getNumTypes();

	int iNumAdded = 0;

	CBotMenu* pCurrent = this;

	for (unsigned i = 0; i < iNumTypes; i++)
	{
		if (!CWaypointTypes::getTypeByIndex(i)->forMod(iMod))
			continue;

		pCurrent->addMenuItem(new CWaypointFlagShowMenuItem(i));
		iNumAdded++;

		if (iNumAdded > 7 || i == iNumTypes - 1)
		{
			CBotMenuItem* back = new CBotGotoMenuItem("Back...", pParent);
			//	make a new menu
			pParent = pCurrent;

			if (iNumAdded > 7 && i < iNumTypes - 1)
			{
				pCurrent = new CBotMenu();
				pCurrent->setCaption("Show Waypoint Flags (More)");
				pParent->addMenuItem(new CBotGotoMenuItem("More...", pCurrent));
			}

			pParent->addMenuItem(back);

			iNumAdded = 0; // reset
		}
	}
}

const char* CWaypointFlagShowMenu::getCaption(CClient* pClient, WptColor& color)
{
	if (pClient->isShowingAllWaypoints())
	{
		snprintf(m_szCaption, sizeof(m_szCaption), "Showing All Waypoints (change)");
		color = WptColor::white;
	}
	else
	{
		snprintf(m_szCaption, sizeof(m_szCaption), "Showing Only Some Waypoints (change)");
		color = CWaypointTypes::getColour(pClient->getShowWaypointFlags());
	}

	return m_szCaption;
}

const char* CWaypointFlagShowMenuItem::getCaption(CClient* pClient, WptColor& color)
{
	const CWaypointType* type = CWaypointTypes::getTypeByIndex(m_iFlag);

	color = type->getColour();

	snprintf(m_szCaption, sizeof(m_szCaption), "[%s] %s", pClient->isShowingAllWaypoints() || pClient->isShowingWaypoint(type->getBits()) ? "showing" : "hiding", type->getName());

	return m_szCaption;
}

void CWaypointFlagShowMenuItem::activate(CClient* pClient)
{
	const CWaypointType* type = CWaypointTypes::getTypeByIndex(m_iFlag);

	// toggle
	if (pClient->isShowingWaypoint(type->getBits()))
		pClient->dontShowWaypoints(type->getBits());
	else
		pClient->showWaypoints(type->getBits());
}

void CBotMenuItem::freeMemory()
{
	// do nothing
}

void CBotMenu::freeMemory()
{
	for (CBotMenuItem* temp : m_MenuItems)
	{
		temp->freeMemory();
		delete temp;
	}
}

void CBotMenuList::freeMemory()
{
	for (CBotMenu* temp : m_MenuList)
	{
		temp->freeMemory();
		delete temp;
	}
}

const char* CPathWaypointDeleteToMenuItem::getCaption(CClient* pClient, WptColor& color)
{
	const int iWpt = pClient->currentWaypoint();

	color = WptColor::white;

	if (iWpt == -1)
	{
		std::strcpy(m_szCaption, "No Waypoint");
		return m_szCaption;
	}

	const CWaypoint* pWaypoint = CWaypoints::getWaypoint(iWpt);

	if (pWaypoint == nullptr)
	{
		std::strcpy(m_szCaption, "Invalid Waypoint");
		return m_szCaption;
	}

	snprintf(m_szCaption, sizeof(m_szCaption), "Delete Paths To This Waypoint (%d)", pWaypoint->numPathsToThisWaypoint());
	
	return m_szCaption;
}

void CPathWaypointDeleteToMenuItem::activate(CClient* pClient)
{
	if (pClient->currentWaypoint() != -1)
		CWaypoints::deletePathsTo(pClient->currentWaypoint());
}

const char* CPathWaypointDeleteFromMenuItem::getCaption(CClient* pClient, WptColor& color)
{
	const int iWpt = pClient->currentWaypoint();

	color = WptColor::white;

	if (iWpt == -1)
	{
		std::strcpy(m_szCaption, "No Waypoint");
		return m_szCaption;
	}

	const CWaypoint* pWaypoint = CWaypoints::getWaypoint(iWpt);

	if (pWaypoint == nullptr)
	{
		std::strcpy(m_szCaption, "Invalid Waypoint");
		return m_szCaption;
	}

	snprintf(m_szCaption, sizeof(m_szCaption), "Delete Paths From This Waypoint (%d)", pWaypoint->numPaths());

	return m_szCaption;
}

void CPathWaypointDeleteFromMenuItem::activate(CClient* pClient)
{
	if (pClient->currentWaypoint() != -1)
		CWaypoints::deletePathsFrom(pClient->currentWaypoint());
}