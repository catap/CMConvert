/*
    Copyright 2003-2005 Brian Smith (brian@smittyware.com)
    This file is part of CMConvert.
    
    CMConvert is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    CMConvert is distributed in the hope that it will be useful,  
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with CMConvert; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "common.h"
#include "wplist.h"

CWPData::CWPData()
{
	m_bConvert = 0;
	m_bTravelBugs = 0;
	m_bTruncated = 0;
}

void CWPData::Update(CWPData *pData)
{
	m_sRecord = pData->m_sRecord;
	m_sTerrain = pData->m_sTerrain;
	m_sDiff = pData->m_sDiff;
	m_sDesc = pData->m_sDesc;
	m_sSymbol = pData->m_sSymbol;
	m_sContainer = pData->m_sContainer;
	m_sType = pData->m_sType;
	m_sState = pData->m_sState;
	m_sCountry = pData->m_sCountry;
	m_sOwner = pData->m_sOwner;
	m_sURL = pData->m_sURL;
	m_sLinks = pData->m_sLinks;

	m_bTruncated = pData->m_bTruncated;
	m_bTravelBugs = pData->m_bTravelBugs;
	m_bActive = pData->m_bActive;

	m_dLat = pData->m_dLat;
	m_dLon = pData->m_dLon;
}

CWPList::~CWPList()
{
	Clear();
}

void CWPList::Clear()
{
	WPList::iterator iter = m_List.begin();
	while (iter != m_List.end())
	{
		delete *iter;
		iter++;
	}

	m_List.clear();

	m_sCurTS.erase();
}

int CWPList::AlreadyInList(CWPData *pWP)
{
	WPList::iterator iter = m_List.begin();
	while (iter != m_List.end())
	{
		if ((*iter)->m_sRecord == pWP->m_sRecord)
			return 1;

		iter++;
	}

	return 0;
}

CWPData* CWPList::GetByWP(string sWP)
{
	WPList::iterator iter = m_List.begin();
	while (iter != m_List.end())
	{
		CWPData *pData = *iter;
		if (pData->m_sWaypoint == sWP)
			return pData;

		iter++;
	}

	return NULL;
}

void CWPList::AddWP(CWPData *pWP)
{
	if (!AlreadyInList(pWP))
		m_List.push_back(pWP);
}

int CWPList::CompareTimestamps(string sFileTS, int &bNewIsLater)
{
	time_t tcur, tnew, diff;
	int bCurTZ, bNewTZ;
	int bLater = 0;

	tcur = ParseTime(m_sCurTS, bCurTZ);
	tnew = ParseTime(sFileTS, bNewTZ);

	if (tcur == (time_t)(-1))
	{
		if (m_List.begin() != m_List.end())
			return 0;
	}

	if (tnew == (time_t)(-1))
	{
		m_sCurTS.erase();
		return 0;
	}
	else if (m_sCurTS.empty())
		bLater = 1;
	else
	{
		diff = tnew - tcur;

		if (bCurTZ == bNewTZ)
			bLater = (diff > 0);
		else if (diff > (time_t)(1209600L))
			bLater = 1;
	}

	bNewIsLater = bLater;
	if (bLater)
		m_sCurTS = sFileTS;

	return 1;
}

time_t mkgmtime(struct tm *t);

time_t CWPList::ParseTime(string sFileTS, int &rHasTZ)
{
	int year, month, day, hour, min, sec;
	struct tm tv;
	time_t t = (time_t)(-1);
	int bHasTZ = 1;
	time_t tz = 0;

	if (sFileTS.empty())
		return t;

	if (sFileTS[0] == '-')
		sFileTS = sFileTS.substr(1);

	if (sscanf(sFileTS.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month,
		&day, &hour, &min, &sec) != 6)
	{
		return t;
	}

	int tpos = sFileTS.find('T');
	if (tpos == string::npos)
		return t;

	if (sFileTS[sFileTS.size()-1] != 'Z')
	{
		int ppos = sFileTS.find('+', tpos+1);
		int mpos = sFileTS.find('-', tpos+1);
		if (ppos != string::npos)
			tz = atoi(sFileTS.substr(ppos+1).c_str());
		else if (mpos != string::npos)
			tz = -atoi(sFileTS.substr(mpos+1).c_str());
		else
			bHasTZ = 0;
	}

	tv.tm_year = year - 1900;
	tv.tm_mday = day;
	tv.tm_mon = month - 1;
	tv.tm_hour = hour;
	tv.tm_min = min;
	tv.tm_sec = sec;

	t = mkgmtime(&tv);

	rHasTZ = bHasTZ;
	t -= (tz * (time_t)3600);

	return t;
}

void CWPList::AddList(CWPList *pList, string sFileTS)
{
	int bLater;

	if (CompareTimestamps(sFileTS, bLater))
		MergeByTimestamp(pList, bLater);
	else
		MergeByRecordContent(pList);

	pList->m_List.clear();
}

void CWPList::MergeByTimestamp(CWPList *pList, int bLater)
{
	WPList::iterator iter = pList->m_List.begin();
	while (iter != pList->m_List.end())
	{
		CWPData *pWP = (*iter);
		CWPData *pOldWP = GetByWP(pWP->m_sWaypoint);

		if (pOldWP)
		{
			if (bLater)
				pOldWP->Update(pWP);

			delete pWP;
		}
		else
			m_List.push_back(pWP);

		iter++;
	}	
}

void CWPList::MergeByRecordContent(CWPList *pList)
{
	WPList::iterator iter = pList->m_List.begin();
	while (iter != pList->m_List.end())
	{
		CWPData *pWP = (*iter);

		if (!AlreadyInList(pWP))
			m_List.push_back(pWP);
		else
			delete pWP;

		iter++;
	}
}
