/*
    Copyright 2003-2004 Brian Smith (brian@smittyware.com)
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

#ifndef _WPLIST_H_INCLUDED_
#define _WPLIST_H_INCLUDED_

class CWPData
{
public:
	CWPData();

	void Update(CWPData *pData);

	string m_sWaypoint;
	string m_sRecord;
	string m_sTerrain;
	string m_sDiff;
	string m_sDesc;
	string m_sSymbol;
	string m_sContainer;
	string m_sType;
	string m_sState;
	string m_sCountry;
	string m_sOwner;
	string m_sURL;
	string m_sLinks;

	int m_bConvert;	
	int m_bTravelBugs;
	int m_bTruncated;
	int m_bActive;

	double m_dLat;
	double m_dLon;
};

#include <list>
typedef list<CWPData*> WPList;

class CWPList
{
public:
	~CWPList();

	void AddWP(CWPData *pWP);
	void AddList(CWPList *pList, string sFileTS);

	CWPData* GetByWP(string sWP);
	
	void Clear();

	WPList m_List;

private:
	string m_sCurTS;

	int AlreadyInList(CWPData *pWP);
	int CompareTimestamps(string sFileTS, int &bNewIsLater);
	time_t ParseTime(string sFileTS, int &rHasTZ);
	void MergeByRecordContent(CWPList *pList);
	void MergeByTimestamp(CWPList *pList, int bLater);
};

#endif // _WPLIST_H_INCLUDED_
