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

#ifndef _PARSER_H_INCLUDED_
#define _PARSER_H_INCLUDED_

#define CHUNK_SIZE 8192
#define MAX_LOGS_SIZE 8192

#include "wplist.h"

// Field indices
#define FLD_NAME	0
#define FLD_WAYPOINT	1
#define FLD_COORD	2
#define FLD_DESC	3
#define FLD_HINTS	4
#define FLD_TYPE	5
#define FLD_NOTES	6
#define FLD_TERRAIN	7
#define FLD_DIFFICULTY	8
#define FLD_LOGS	9
#define FLD_NAME2	10
#define FLD_TYPE2	11
#define FLD_LAT		12
#define FLD_LON		13
#define FLD_SHRT_DESC	14
#define FLD_LONG_DESC	15
#define FLD_SHRT_HTML	16
#define FLD_LONG_HTML	17
#define FLD_DATETIME	18
#define FLD_OWNER	19
#define FLD_STATE	20
#define FLD_COUNTRY	21
#define FLD_CONTAINER	22
#define FLD_LOG_DATE	23
#define FLD_LOG_TYPE	24
#define FLD_LOG_FINDER	25
#define FLD_LOG_TEXT	26
#define FLD_LOG_ENC	27
#define FLD_LOG_LAT	28
#define FLD_LOG_LON	29
#define FLD_BUGS	30
#define FLD_SYMBOL	31
#define FLD_URL		32
#define FLD_LINKS	33
#define FLD_STATUS	34

#define MAX_END_FIELDS	10
#define MAX_MID_FIELDS	35

class CXMLParser
{
public:
	CXMLParser();

	CWPList *m_pList;

	int m_bLocWarning;
	int m_bEmptyDesc;
	string m_sFileTS;

	// Parser options from command line
	int m_bLocation;
	int m_bContainer;
	int m_bOwner;
	int m_bDate;
	int m_bShowBugs;
	int m_bDecodeHints;
	int m_nMaxLogs;
	int m_nMaxDesc;
	int m_bLogTemplate;
	int m_bQuiet;
	int m_bCacheStatus;
	int m_bStripNameQuotes;

	int ParseFile(string sPath);

private:
	string m_sCurTag, m_sCurData;
	char m_buf[CHUNK_SIZE];
	string m_sRecord;
	string m_sCurPath;
	string m_sFields[MAX_MID_FIELDS];
	string m_sEncoding;
	int m_nCurLogs;
	int m_bHasBugs;
	int m_bUTF8;
	int m_bNonCacheFile;
	int m_bCacheActive;
	int m_bLongDesc;

	static void HandleElemStart(void *data, const char *el, const char 
		**attr);
	static void HandleElemEnd(void *data, const char *el);
	static void HandleCharData(void *data, const char *s, int len);
	static void HandleXmlDecl(void *userData, const char *version,
        	const char *encoding, int standalone);

	static int GetElementInfo(string sPath, int &rField, int &rFlags);
	static void GetAttribute(const char **attr, string sName,
		string &sVal);

	int IsWhitespace(string &rStr);
	void ClearWaypoint();
	void EncodeHints(int nField);
	void FinishWaypointRecord();
	void HTMLToText(string &sStr);
	void ConvertCoords(string &sLat, string &sLon, string &sCoord,
		double &dLat, double &dLon);
	void StripWhitespace(string &rStr);
	void StripQuotes(string &rStr);
	void CompileCacheLog();
	void StripTags(string &sStr);
	void ReformatDate(string &sDate);
	void DecodeUTF8(string &sStr);
	void ExtractURL(string sTag, string sArgs);
	void StoreCacheStatus(string sAvail, string sArchived);
	void FormatFileTS(string sPath);
	void AddLinkToRecord(string sURL);

	void CleanCharRefs(char *szBuf);
	int CheckCharVal(char *pVal);
};

#endif // _PARSER_H_INCLUDED_

