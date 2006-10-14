/*
    Copyright 2003-2006 Brian Smith (brian@smittyware.com)
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
#include "parser.h"
#include "reader.h"
#include "util.h"

extern "C" {
#include <expat.h>
}

// Flags for attributes
#define FF_COORD_ATTR	1	// Coordinates are lat/lon attributes
#define FF_WAYPOINT_ID	2	// Waypoint name is "id" attribute
#define FF_CHECK_HTML	4	// HTML flag of long description
#define FF_FORCE_HTML	8	// Trust us... it's HTML :-)
#define FF_REC_ELEM	16	// Base element of item record
#define FF_LOG_ELEM	32	// Base element of cache log
#define FF_LOG_ENC	64	// Encoded attribute of cache log
#define FF_BUG_NAME	128	// Name of travel bug
#define FF_COORD_LOG	256	// Coordinates from log entry
#define FF_LOC_WARNING	512	// Display warning about LOC file import
#define FF_CACHE_STATUS	1024	// available/archived attrs for caches
#define FF_TIMESTAMP	2048	// GPX file timestamp
#define FF_HREF_ATTR	4096	// GPX 1.1 link (URL in href attr)
#define FF_WPT_URL	8192	// Waypoint URL (GPX 1.0)
#define FF_TERRA_SIZES	16384	// Map TerraCaching size number to string
#define FF_GPX_VERSION	32768	// GPX version attribute

// Geocaching.com.au GPX extensions
static stPathMap s_AUMap[] = {
  { "", -1, FF_CACHE_STATUS },
  { "/name", FLD_NAME2, 0 },
  { "/type", FLD_TYPE2, 0 },
  { "/owner", FLD_OWNER, 0 },
  { "/container", FLD_CONTAINER, 0 },
  { "/locale", FLD_LOCALE, 0 },
  { "/state", FLD_STATE, 0 },
  { "/country", FLD_COUNTRY, 0 },
  { "/difficulty", FLD_DIFFICULTY, 0 },
  { "/terrain", FLD_TERRAIN, 0 },
  { "/summary", FLD_SHRT_DESC, FF_FORCE_HTML },
  { "/description", FLD_LONG_DESC, FF_FORCE_HTML },
  { "/hints", FLD_HINTS, 0 },
  { "/logs/log", -1, FF_LOG_ELEM },
  { "/logs/log/time", FLD_LOG_DATE, 0 },
  { "/logs/log/type", FLD_LOG_TYPE, 0 },
  { "/logs/log/geocacher", FLD_LOG_FINDER, 0 },
  { "/logs/log/text", FLD_LOG_TEXT, 0 },
  { "/logs/log/coords", -1, FF_COORD_LOG },
  { NULL, 0, 0 }
};

// Groundspeak GPX extensions
static stPathMap s_GSMap[] = {
  { "", -1, FF_CACHE_STATUS },
  { "/name", FLD_NAME2, 0 },
  { "/type", FLD_TYPE2, 0 },
  { "/owner", FLD_OWNER, 0 },
  { "/container", FLD_CONTAINER, 0 },
  { "/state", FLD_STATE, 0 },
  { "/country", FLD_COUNTRY, 0 },
  { "/difficulty", FLD_DIFFICULTY, 0 },
  { "/terrain", FLD_TERRAIN, 0 },
  { "/short_description", FLD_SHRT_DESC, FF_CHECK_HTML },
  { "/long_description", FLD_LONG_DESC, FF_CHECK_HTML },
  { "/encoded_hints", FLD_HINTS, 0 },
  { "/logs/log", -1, FF_LOG_ELEM },
  { "/logs/log/date", FLD_LOG_DATE, 0 },
  { "/logs/log/type", FLD_LOG_TYPE, 0 },
  { "/logs/log/finder", FLD_LOG_FINDER, 0 },
  { "/logs/log/text", FLD_LOG_TEXT, FF_LOG_ENC },
  { "/logs/log/log_wpt", -1, FF_COORD_LOG },
  { "/travelbugs/travelbug/name", -1, FF_BUG_NAME },
  { NULL, 0, 0 }
};

// TerraCache GPX extensions
static stPathMap s_TerraMap[] = {
  { "/name", FLD_NAME2, 0 },
  { "/owner", FLD_OWNER, 0 },
  { "/style", FLD_TYPE2, 0 },
  { "/size", FLD_CONTAINER, FF_TERRA_SIZES },
  { "/physical_challenge", FLD_TERRAIN, 0 },
  { "/mental_challenge", FLD_DIFFICULTY, 0 },
  { "/camo_challenge", FLD_CAMO_DIFF, 0 },
  { "/country", FLD_COUNTRY, 0 },
  { "/state", FLD_STATE, 0 },
  { "/description", FLD_LONG_DESC, FF_FORCE_HTML },
  { "/hint", FLD_HINTS, FF_FORCE_HTML },
  { "/logs/log", -1, FF_LOG_ELEM },
  { "/logs/log/date", FLD_LOG_DATE, 0 },
  { "/logs/log/type", FLD_LOG_TYPE, 0 },
  { "/logs/log/user", FLD_LOG_FINDER, 0 },
  { "/logs/log/entry", FLD_LOG_TEXT, FF_FORCE_HTML },
  { NULL, 0, 0 }
};

// GPX extension mapping
static stExtMap s_ExtMap[] = {
  { "http://www.groundspeak.com/cache/", 1, "/cache", s_GSMap },
  { "http://www.terracaching.com/gpx/",	1, "/terracache", s_TerraMap },
  { "http://geocaching.com.au/geocache/1", 0, "/geocache", s_AUMap },
  { NULL, 0, NULL }
};

// GPX/LOC base field mapping
static stPathMap s_BaseMap[] = {
  // GPX 1.0
  { "/gpx", -1, FF_GPX_VERSION },
  { "/gpx/time", -1, FF_TIMESTAMP },
  { "/gpx/wpt", -1, FF_REC_ELEM | FF_COORD_ATTR },
  { "/gpx/wpt/name", FLD_WAYPOINT, 0 },
  { "/gpx/wpt/desc", FLD_NAME, 0 },
  { "/gpx/wpt/cmt", FLD_COMMENT, 0 },
  { "/gpx/wpt/type", FLD_TYPE, 0 },
  { "/gpx/wpt/time", FLD_DATETIME, 0 },
  { "/gpx/wpt/sym", FLD_SYMBOL, 0 },
  { "/gpx/wpt/url", -1, FF_WPT_URL },
  // GPX 1.1
  { "/gpx/metadata/time", -1, FF_TIMESTAMP },
  { "/gpx/wpt/link", -1, FF_HREF_ATTR },
  // LOC
  { "/loc/waypoint", -1, FF_REC_ELEM | FF_LOC_WARNING },
  { "/loc/waypoint/name", FLD_NAME, FF_WAYPOINT_ID },
  { "/loc/waypoint/coord", -1, FF_COORD_ATTR },
  { "/loc/waypoint/type", FLD_TYPE, 0 },
  // Terminator record
  { NULL, 0, 0 }
};

CXMLParser::CXMLParser()
{
	m_bDate = 0;
	m_bLocation = 0;
	m_bOwner = 0;
	m_bContainer = 0;
	m_nMaxLogs = 0;
	m_bShowBugs = 0;
	m_nMaxDesc = 3090;
	m_bDecodeHints = 0;
	m_bLogTemplate = 0;
	m_bQuiet = 0;
	m_bStripNameQuotes = 0;

	m_nCurLogs = 0;
	m_bHasBugs = 0;
	m_bLocWarning = 0;
	m_bEmptyDesc = 1;
	m_bNonCacheFile = 0;
	m_bCacheStatus = 0;
	m_bCacheActive = 1;
	m_bLongDesc = 0;
	m_dGpxVer = 0;
	m_pExtensions = NULL;
	m_bHtmlFlag = 0;

	m_sCurPath.erase();
	m_sFileTS.erase();

	ClearWaypoint();
}

void CXMLParser::CheckForGPXExtensions(const char **attr)
{
	if (m_dGpxVer < 1.0)
		return;

	int bFound = 0;
	for (int i=0; attr[i]; i+=2)
	{
		string sName = attr[i], sVal = attr[i+1];
		if (sName != "xmlns" && sName.substr(0,6) != "xmlns:")
			continue;

		CUtil::LowercaseString(sVal);

		stExtMap *pExt = s_ExtMap;
		while (pExt->szURI)
		{
			if (pExt->bPrefix)
			{
				if (sVal.substr(0, strlen(pExt->szURI)) ==
					pExt->szURI)
				{
					m_pExtensions = pExt;
					bFound = 1;
					break;
				}
			}
			else if (sVal == pExt->szURI)
			{
				m_pExtensions = pExt;
				bFound = 1;
				break;
			}

			pExt++;
		}

		if (bFound)
			break;
	}

	if (bFound)
	{
		m_sExtBase = (m_dGpxVer > 1.0) ? "/gpx/wpt/extensions" : 
			"/gpx/wpt";
		m_sExtBase += m_pExtensions->szExtElem;
	}
}

void CXMLParser::GetAttribute(const char **attr, string sName, string &sVal)
{
	sVal.erase();

	int i;
	for (i=0; attr[i]; i += 2)
	{
		if (sName == attr[i])
		{
			sVal = attr[i+1];
			break;
		}
	}
}

int CXMLParser::GetElementInfo(string sPath, int &rField, int32_t &rFlags)
{
	stPathMap *pField;

	// Check base field map
	pField = s_BaseMap;
	while (pField->szName)
	{
		if (sPath == pField->szName)
		{
			rField = pField->nField;
			rFlags = pField->nFlags;
			return 1;
		}

		pField++;
	}

	if (!m_pExtensions)
		return 0;	// No extensions discovered... yet

	if (sPath.substr(0, m_sExtBase.size()) != m_sExtBase)
		return 0;	// Base path doesn't match

	// Check extension field map
	pField = m_pExtensions->pMap;
	while (pField->szName)
	{
		string sCheck = m_sExtBase;
		sCheck += pField->szName;

		if (sPath == sCheck)
		{
			rField = pField->nField;
			rFlags = pField->nFlags;
			return 1;
		}

		pField++;
	}

	return 0;
}

void CXMLParser::HandleElemStart(void *data, const char *el, const char **attr)
{
	CXMLParser *pParser = (CXMLParser*)data;

	string elem = el;
	int pos = elem.find(':');
	if (pos != string::npos)
		elem = elem.substr(pos+1);

	pParser->m_sCurTag = elem;
	pParser->m_sCurData.erase();
	pParser->m_sCurPath += '/';
	pParser->m_sCurPath += elem;

	if (!pParser->m_pExtensions)
		pParser->CheckForGPXExtensions(attr);

	int bData, nField;
	int32_t nFlags;
	bData = pParser->GetElementInfo(pParser->m_sCurPath, nField, nFlags);
	if (!bData)
		return;

	if (nFlags & FF_GPX_VERSION)
	{
		string sVer;
		GetAttribute(attr, "version", sVer);
		if (!sVer.empty())
			pParser->m_dGpxVer = strtod(sVer.c_str(), NULL);

		if (!pParser->m_pExtensions)
			pParser->CheckForGPXExtensions(attr);
	}

	if (nFlags & FF_REC_ELEM)
		pParser->ClearWaypoint();

	if (nFlags & FF_WAYPOINT_ID)
		GetAttribute(attr, "id", pParser->m_sFields[FLD_WAYPOINT]);

	if (nFlags & FF_CHECK_HTML)
	{
		string sVal;
		GetAttribute(attr, "html", sVal);
		CUtil::LowercaseString(sVal);

		pParser->m_bHtmlFlag = 
			(sVal == "true" || sVal == "yes" || sVal == "1");
	}

	if (nFlags & FF_HREF_ATTR)
	{
		string sURL;
		GetAttribute(attr, "href", sURL);
		pParser->AddLinkToRecord(sURL);
	}

	if (nFlags & FF_COORD_ATTR)
	{
		GetAttribute(attr, "lat", pParser->m_sFields[FLD_LAT]);
		GetAttribute(attr, "lon", pParser->m_sFields[FLD_LON]);
	}

	if (nFlags & FF_COORD_LOG)
	{
		GetAttribute(attr, "lat", pParser->m_sFields[FLD_LOG_LAT]);
		GetAttribute(attr, "lon", pParser->m_sFields[FLD_LOG_LON]);
	}

	if (nFlags & FF_LOG_ELEM)
	{
		int i;
		for (i=FLD_LOG_DATE; i<=FLD_LOG_LON; i++)
			pParser->m_sFields[i].erase();
	}

	if (nFlags & FF_LOG_ENC)
		GetAttribute(attr, "encoded",
			pParser->m_sFields[FLD_LOG_ENC]);

	if (nFlags & FF_LOC_WARNING)
		pParser->m_bLocWarning = 1;

	if (nFlags & FF_CACHE_STATUS)
	{
		string sStatus;
		GetAttribute(attr, "status", sStatus);

		if (!sStatus.empty())
		{	// Geocaching.com.au
			pParser->m_sFields[FLD_STATUS] = sStatus;
			pParser->m_bCacheActive =
				(sStatus == "Available") ? 1 : 0;
		}
		else
		{	// Groundspeak
			string sAvail, sArchived;
			GetAttribute(attr, "available", sAvail);
			GetAttribute(attr, "archived", sArchived);
			pParser->StoreCacheStatus(sAvail, sArchived);
		}
	}
}

// Map of redundant Unicode characters
typedef struct stCharMap
{
	uint32_t ch;
	char *str;
} stCharMap;
static const stCharMap aCharMap[] = {
	{ 0x0152, "\x8c" },
	{ 0x0153, "\x9c" },
	{ 0x0160, "\x8a" },
	{ 0x0161, "\x9a" },
	{ 0x0178, "\x9f" },
	{ 0x0192, "\x83" },
	{ 0x02c6, "\x88" },
	{ 0x02dc, "\x98" },
	{ 0x2013, "\x96" },
	{ 0x2014, "\x97" },
	{ 0x2018, "\x91" },
	{ 0x2019, "\x92" },
	{ 0x201A, "\x82" },
	{ 0x201E, "\x84" },
	{ 0x201c, "\x93" },
	{ 0x201d, "\x94" },
	{ 0x2020, "\x86" },
	{ 0x2021, "\x87" },
	{ 0x2022, "\x95" },
	{ 0x2026, "..." },
	{ 0x2030, "\x89" },
	{ 0x2039, "\x8b" },
	{ 0x203a, "\x9b" },
	{ 0x20AC, "\x80" },
	{ 0x2122, "\x99" },
	{ 0, 0 }
};

void CXMLParser::DecodeUTF8(string &sStr)
{
	int i, n = sStr.size();
	string sTmp;
	unsigned long lch;
	int nch = 0;

	sTmp.empty();
	for (i=0; i<n; i++)
	{
		char ch = sStr[i];
		unsigned char uch = *(unsigned char*)(&ch);

		if (nch > 0)
		{
			lch = (lch * 64) + (uch - 128);
			nch--;

			if (nch > 0)
				continue;
		}
		else if (uch >= 128)
		{
			if (uch >= 252)
			{
				lch = uch - 252;
				nch = 5;
			}
			else if (uch >= 248)
			{
				lch = uch - 248;
				nch = 4;
			}
			else if (uch >= 240)
			{
				lch = uch - 240;
				nch = 3;
			}
			else if (uch >= 224)
			{
				lch = uch - 224;
				nch = 2;
			}
			else if (uch >= 192)
			{
				lch = uch - 192;
				nch = 1;
			}

			continue;
		}
		else
			lch = uch;

		if (lch < 256)
		{	// ASCII character code
			uch = (unsigned char)(lch & 0xff);
			sTmp += *(char*)(&uch);
		}
		else
		{	// Remap Unicode characters to ASCII
			int i;
			int bFound;
			const stCharMap *pEntry;

			bFound = 0;
			for (i=0; aCharMap[i].ch != 0; i++)
			{
				pEntry = &aCharMap[i];
				if (lch == pEntry->ch)
				{
					bFound = 1;
					break;
				}
			}

			if (bFound)
				sTmp += pEntry->str;
		}
	}

	sStr = sTmp;
}

void CXMLParser::TranslateTerraSizes(string &rSize)
{
	int nSize = atoi(rSize.c_str());
	switch (nSize)
	{
	case 1: rSize = "Large"; break;
	case 2: rSize = "Medium"; break;
	case 3: rSize = "Small"; break;
	case 4: rSize = "Mini"; break;
	case 5: rSize = "Micro"; break;
	}
}

void CXMLParser::HandleElemEnd(void *data, const char *el)
{
	CXMLParser *pParser = (CXMLParser*)data;

	int bData, nField;
	int32_t nFlags;
	bData = pParser->GetElementInfo(pParser->m_sCurPath, nField, nFlags);
	pParser->DecodeUTF8(pParser->m_sCurData);

	if (bData)
	{
		if (nFlags & FF_FORCE_HTML)
		{
			pParser->HTMLToText(pParser->m_sCurData);
			// TerraCaching.com likes to do weird things with
			// <br> tags
			pParser->StripTags(pParser->m_sCurData);
		}

		if ((nFlags & FF_CHECK_HTML) && pParser->m_bHtmlFlag)
			pParser->HTMLToText(pParser->m_sCurData);

		if (nFlags & FF_TERRA_SIZES)
			TranslateTerraSizes(pParser->m_sCurData);

		if (nField != -1)
			pParser->m_sFields[nField] = pParser->m_sCurData;

		if (nFlags & FF_REC_ELEM)
			pParser->FinishWaypointRecord();

		if (nFlags & FF_LOG_ELEM)
			pParser->CompileCacheLog();

		if (nFlags & FF_WPT_URL)
			pParser->AddLinkToRecord(pParser->m_sCurData);

		if (nFlags & FF_BUG_NAME)
		{
			if (!pParser->m_sFields[FLD_BUGS].empty())
				pParser->m_sFields[FLD_BUGS] += ", ";

			pParser->m_sFields[FLD_BUGS] += pParser->m_sCurData;
			pParser->m_bHasBugs = 1;
		}

		if (nFlags & FF_TIMESTAMP)
			pParser->m_sFileTS = pParser->m_sCurData;
	}

	pParser->m_sCurData.erase();

	int nPos = pParser->m_sCurPath.rfind('/');
	pParser->m_sCurPath = pParser->m_sCurPath.substr(0, nPos);
}

void CXMLParser::HandleCharData(void *data, const char *s, int len)
{
	CXMLParser *pParser = (CXMLParser*)data;
	int i;

	for (i=0; i<len; i++)
		pParser->m_sCurData += s[i];
}

void CXMLParser::StoreCacheStatus(string sAvail, string sArchived)
{
	CUtil::LowercaseString(sAvail);
	CUtil::LowercaseString(sArchived);

	string sStatus;
	if (sArchived == "true")
		sStatus = "Archived";
	else if (sAvail == "true")
		sStatus = "Active";
	else
		sStatus = "Inactive";

	if (sStatus != "Active")
		m_bCacheActive = 0;

	m_sFields[FLD_STATUS] = sStatus;
}

void CXMLParser::ReformatDate(string &sDate)
{
#if HAVE_LOCALE_H && HAVE_SETLOCALE && HAVE_STRFTIME
	struct tm tmi;
	char buf[20];

#ifdef HAVE_MEMSET
        memset(&tmi, 0, sizeof(tmi));
#else
        bzero(&tmi, sizeof(tmi));
#endif

	sscanf(sDate.c_str(), "%d-%d-%d", &(tmi.tm_year), &(tmi.tm_mon),
		&(tmi.tm_mday));
	tmi.tm_year -= 1900;
	tmi.tm_mon--;

	strftime(buf, 20, "%x", &tmi);
	sDate = buf;
#endif
}

void CXMLParser::CompileCacheLog()
{
	string sLog, sCoords;
	double dLat, dLon;

	sCoords.erase();
	ConvertCoords(m_sFields[FLD_LOG_LAT], m_sFields[FLD_LOG_LON],
		sCoords, dLat, dLon);

	// Parse timestamp... we just need the date
	string &rDT = m_sFields[FLD_LOG_DATE];
	int i = rDT.find('T');
	if (i != string::npos)
		rDT = rDT.substr(0, i);

	ReformatDate(rDT);
	sLog = rDT + "\002";

	// Add the log header (finder and type)
	sLog += m_sFields[FLD_LOG_FINDER] + " (";
	sLog += m_sFields[FLD_LOG_TYPE] + ")\n";

	// Add coordinates from log (if they are given)
	if (!sCoords.empty())
		sLog += sCoords + "\n";

	sLog += "\n";

	// Add the log text (after encoding it, as appropriate)
	CUtil::StripWhitespace(m_sFields[FLD_LOG_TEXT]);
	if (!m_bDecodeHints && (m_sFields[FLD_LOG_ENC] == "True"))
		EncodeHints(FLD_LOG_TEXT);
	sLog += m_sFields[FLD_LOG_TEXT];

	// Check size and number of logs against limits
	if (sLog.size() > 3072)
	{
		sLog = sLog.substr(0, 3060);
		sLog += "\n[truncated]";
	}
	if ((m_sFields[FLD_LOGS].size() + sLog.size()) > MAX_LOGS_SIZE)
		return;
	if (m_nCurLogs >= m_nMaxLogs)
		return;

	StripTags(sLog);
	CUtil::StripWhitespace(sLog);

	// Append log to list
	if (!m_sFields[FLD_LOGS].empty())
		m_sFields[FLD_LOGS] += "\002";
	m_sFields[FLD_LOGS] += sLog;
	m_nCurLogs++;
}

void CXMLParser::ClearWaypoint()
{
	int i;

	for (i=0; i<MAX_MID_FIELDS; i++)
		m_sFields[i].erase();

	m_nCurLogs = 0;
	m_bHasBugs = 0;
	m_bCacheActive = 1;
}

void CXMLParser::AddLinkToRecord(string sURL)
{
	string &rWptURL = m_sFields[FLD_URL];
	if (rWptURL.empty() && (
		sURL.substr(0,49) ==
			"http://www.geocaching.com/seek/cache_details.aspx" ||
		sURL.substr(0,37) ==
			"http://www.terracaching.com/xlink.cgi" ||
		sURL.substr(0,41) ==
			"http://www.terracaching.com/viewcache.cgi" ||
		sURL.substr(0,31) ==
			"http://geocaching.com.au/cache/"
	))
	{	// Detected a cache page link
		rWptURL = sURL;
		return;
	}

	sURL += '\001';
	if (m_sFields[FLD_LINKS].find(sURL) == string::npos)
		m_sFields[FLD_LINKS] += sURL;
}

void CXMLParser::ExtractURL(string sTag, string sArgs)
{
	int bImg = 0;

	if (sTag == "img" || sTag == "image")
		bImg = 1;
	else if (sTag != "a")
		return;

	CUtil::StripWhitespace(sArgs);

	// Find URL attribute
	string sSearch = sArgs;
	CUtil::LowercaseString(sSearch);
	int nIndex = sSearch.find(bImg ? "src=" : "href=");
	if (nIndex == string::npos)
		return;

	// Extract URL from attribute, stripping quotes if needed
	string sURL = sArgs.substr(nIndex + (bImg ? 4 : 5));
	CUtil::StripWhitespace(sURL);
	char chQuote = ' ';
	if (sURL[0] == '"' || sURL[0] == '\'')
	{
		chQuote = sURL[0];
		sURL = sURL.substr(1);
	}
	nIndex = sURL.find(chQuote);
	if (nIndex != string::npos)
		sURL = sURL.substr(0, nIndex);

	CUtil::StripWhitespace(sURL);
	AddLinkToRecord(sURL);
}

// Map of named HTML entities
typedef struct stEntMap
{
	char *name;
	uint32_t ch;
} stEntMap;
static const stEntMap aEntMap[] = {
	{ "nbsp", ' ' },
	{ "gt", '>' },
	{ "lt", '<' },
	{ "amp", '&' },
	{ "quot", '\"' },
	{ "iexcl", 161 },
	{ "cent", 162 },
	{ "pound", 163 },
	{ "curren", 164 },
	{ "yen", 165 },
	{ "brvbar", 166 },
	{ "sect", 167 },
	{ "uml", 168 },
	{ "copy", 169 },
	{ "ordf", 170 },
	{ "laquo", 171 },
	{ "not", 172 },
	{ "shy", 173 },
	{ "reg", 174 },
	{ "macr", 175 },
	{ "deg", 176 },
	{ "plusmn", 177 },
	{ "sup2", 178 },
	{ "sup3", 179 },
	{ "acute", 180 },
	{ "micro", 181 },
	{ "para", 182 },
	{ "middot", 183 },
	{ "cedil", 184 },
	{ "sup1", 185 },
	{ "ordm", 186 },
	{ "raquo", 187 },
	{ "frac14", 188 },
	{ "frac12", 189 },
	{ "frac34", 190 },
	{ "iquest", 191 },
	{ "times", 215 },
	{ "szlig", 223 },
	{ "agrave", 224 },
	{ "aacute", 225 },
	{ "acirc", 226 },
	{ "atilde", 227 },
	{ "auml", 228 },
	{ "aring", 229 },
	{ "aelig", 230 },
	{ "ccedil", 231 },
	{ "egrave", 232 },
	{ "eacute", 233 },
	{ "ecirc", 234 },
	{ "euml", 235 },
	{ "igrave", 236 },
	{ "iacute", 237 },
	{ "icirc", 238 },
	{ "iuml", 239 },
	{ "eth", 240 },
	{ "ntilde", 241 },
	{ "ograve", 242 },
	{ "oacute", 243 },
	{ "ocirc", 244 },
	{ "otilde", 245 },
	{ "ouml", 246 },
	{ "divide", 247 },
	{ "oslash", 248 },
	{ "ugrave", 249 },
	{ "uacute", 250 },
	{ "ucirc", 251 },
	{ "uuml", 252 },
	{ "yacute", 253 },
	{ "thorn", 254 },
	{ "yuml", 255 },
	{ "oelig", 339 },
	{ "scaron", 353 },
	{ "fnof", 402 },
	{ "circ", 710 },
	{ "tilde", 732 },
	{ "ndash", 8211 },
	{ "mdash", 8212 },
	{ "lsquo", 8216 },
	{ "rsquo", 8217 },
	{ "sbquo", 8218 },
	{ "ldquo", 8220 },
	{ "rdquo", 8221 },
	{ "bdquo", 8222 },
	{ "dagger", 8224 },
	{ "bull", 8226 },
	{ "hellip", 8230 },
	{ "permil", 8240 },
	{ "lsaquo", 8249 },
	{ "rsaquo", 8250 },
	{ "trade", 8482 },
	{ NULL, 0 }
};

void CXMLParser::DecodeEntity(string &sEnt, string &sValue)
{
	sValue.erase();
	int nValue = 0;

	const char *pStr = sEnt.c_str();
	if (*pStr == '#')
	{
		if (pStr[1] == 'x')
			nValue = strtoul(pStr+2, NULL, 16);
		else
			nValue = strtoul(pStr+1, NULL, 10);
	}
	else
	{
		stEntMap *pEnt = (stEntMap*)aEntMap;
		while (pEnt->name)
		{
			if (pEnt->name == sEnt)
			{
				nValue = pEnt->ch;
				break;
			}

			pEnt++;
		}
	}

	if (nValue > 0)
	{
		if (nValue >= 256)
		{
			stCharMap *pChar = (stCharMap*)aCharMap;
			while (pChar->ch)
			{
				if (pChar->ch == nValue)
				{
					sValue = pChar->str;
					break;
				}

				pChar++;
			}
		}
		else
			sValue = (char)nValue;
	}
}

void CXMLParser::HTMLToText(string &sStr)
{
	string sResult;
	int i, n;
	string sTag, sEntity, sArgs;
	int bTag = 0, bWS = 1, bTagName = 0, bEntity = 0, bPre = 0;
	int bListItem = 0;

	n = sStr.size();
	for (i=0; i<n; i++)
	{
		char ch = sStr[i];
		int bWSChar = (strchr(" \t\r\n", ch) != NULL);

		if (bTag)
		{
			if (bTagName)
			{
				if (bWSChar || (ch == '>'))
					bTagName = 0;
				else 
					sTag += ch;
			}

			if (ch == '>')
			{
				bTag = 0;
				CUtil::LowercaseString(sTag);

				int bTooMuch = 0;
				int nLen = sResult.size();
				if (nLen >= 2)
				{
					if (sResult.substr(nLen-2, 2) == "\n\n")
						bTooMuch = 1;
				}
				else if (nLen == 0)
					bTooMuch = 1;

				int bNL = 0;
				if (nLen >= 1 &&
					sResult.substr(nLen-1) == "\n")
					bNL = 1;

				int bHandled = 1;
				if (sTag == "p")
				{
					if (!bTooMuch)
						sResult += "\n\n";
				}
				else if (sTag == "/p")
				{
					if (!bTooMuch)
						sResult += "\n";
				}
				else if (sTag == "br")
				{
					if (!bTooMuch)
						sResult += "\n";
				}
				else if (sTag == "li")
				{
					if (!bTooMuch && bListItem)
						sResult += "\n";

					sResult += "* ";
					bListItem = 1;
				}
				else if (sTag == "/li")
				{
					bListItem = 0;
					if (!bTooMuch)
						sResult += "\n";
				}
				else if (sTag == "ul" || sTag == "ol")
				{
					if (!bTooMuch)
						sResult += "\n";
				}
				else if (sTag == "/ul" || sTag == "/ol")
				{
					bListItem = 0;
					if (!bTooMuch)
						sResult += "\n";
				}
				else if (sTag == "table")
				{
					if (!bTooMuch)
						sResult += "\n\n";
				}
				else if (sTag == "/table")
				{
					if (!bTooMuch)
						sResult += "\n";
				}
				else if (sTag == "/tr" || sTag == "/caption")
				{
					if (!bNL)
						sResult += "\n";
				}
				else if (sTag == "/th" || sTag == "/td")
				{
					if (!bWS)
						sResult += " ";
				}
				else
					bHandled = 0;

				if (bHandled)
					bWS = 1;

				if (sTag == "pre")
					bPre = 1;
				else if (sTag == "/pre")
					bPre = 0;

				ExtractURL(sTag, sArgs);
			}
			else if (!bTagName)
			{
				if (bWSChar)
					sArgs += ' ';
				else
					sArgs += ch;
			}
		}
		else if (bEntity)
		{
			if (bWSChar)
			{
				bEntity = 0;
				sResult += "&";
				sResult += sEntity + ch;
			}
			else if (ch == ';')
			{
				string sValue;

				bEntity = 0;
				CUtil::LowercaseString(sEntity);
				DecodeEntity(sEntity, sValue);
				sResult += sValue;
			}
			else
				sEntity += ch;
		}
		else if (ch == '<')
		{
			sTag.erase();
			sArgs.erase();
			bTag = 1;
			bTagName = 1;
		}
		else if (ch == '&')
		{
			sEntity.erase();
			bEntity = 1;
			bWS = 0;
		}
		else if (bWSChar && !bPre)
		{
			if (!bWS)
			{
				sResult += ' ';
				bWS = 1;
			}
		}
		else
		{
			bWS = 0;
			sResult += ch;
		}
	}

	sStr = sResult;
}

void CXMLParser::EncodeHints(int nField)
{
	static char *szLower13 = "nopqrstuvwxyzabcdefghijklm";
	static char *szUpper13 = "NOPQRSTUVWXYZABCDEFGHIJKLM";
	int bImmune = 0;
	int n;

	string &rHints = m_sFields[nField];
	n = rHints.size();
	while (n--)
	{
		char ch = rHints[n];
		if (ch == ']')
			bImmune = 1;
		else if (ch == '[')
			bImmune = 0;
		else if (!bImmune)
		{
			if (ch >= 'A' && ch <= 'Z')
				ch = szUpper13[ch - 'A'];
			else if (ch >= 'a' && ch <= 'z')
				ch = szLower13[ch - 'a'];

			rHints[n] = ch;
		}
	}
}

void CXMLParser::ConvertCoords(string &sLat, string &sLon, string &sCoord,
	double &dLat, double &dLon)
{
	double dLatD, dLatM, dLonD, dLonM;
	static char buf[64];
	int bWestLon = 0, bSouthLat = 0;

	sCoord.erase();
	if (sLat.empty() || sLon.empty())
		return;

	dLat = dLatM = atof(sLat.c_str());
	if (dLatM < 0.0)
	{
		bSouthLat = 1;
		dLatM = -dLatM;
	}

	dLatD = (int)dLatM;
	dLatM -= dLatD;
	dLatM *= 60;

	dLon = dLonM = atof(sLon.c_str());
	if (dLonM < 0.0)
	{
		bWestLon = 1;
		dLonM = -dLonM;
	}

	dLonD = (int)dLonM;
	dLonM -= dLonD;
	dLonM *= 60;

	sprintf(buf, "%c %d° %06.3f %c %03d° %06.3f",
		bSouthLat ? 'S' : 'N', (int)dLatD, dLatM,
		bWestLon ? 'W' : 'E', (int)dLonD, dLonM);
	sCoord = buf;
}

void CXMLParser::StripQuotes(string &rStr)
{
	int nCount = 0;
	int bDouble = 0;

	while (strchr("\"'", rStr[0]) != NULL && rStr.size() > 0)
	{
		if (rStr[0] == '"')
			bDouble = 1;

		rStr = rStr.substr(1);
		nCount++;
	}

	if (bDouble)
	{
		int nPos = rStr.find('"');
		if (nPos != string::npos)
		{
			string sTemp = rStr.substr(0, nPos);
			if (nPos != (rStr.size() - 1))
				sTemp += rStr.substr(nPos+1);
			rStr = sTemp;
		}
	}

	if (nCount > 0)
	{
		int nLen = rStr.size();
		while (nLen > 0 && strchr("\"'", rStr[nLen-1]) != NULL)
			rStr = rStr.substr(0, --nLen);
	}
}

int CXMLParser::IsWhitespace(string &rStr)
{
	int i, n = rStr.size();

	if (n == 0)
		return 1;

	for (i=0; i<n; i++)
	{
		if (strchr(" \t\r\n", rStr[i]) == NULL)
			return 0;
	}

	return 1;
}

void CXMLParser::StripTags(string &sStr)
{
	string sTmp;
	int i, n = sStr.size();
	int bTag = 0, bLT = 0;

	for (i=0; i<n; i++)
	{
		char ch = sStr[i];

		if (bTag)
		{
			if (ch == '>')
				bTag = 0;

			continue;
		}
		else if (bLT)
		{
			bLT = 0;

			if (ch == '/' || (ch >= 'a' && ch <= 'z') ||
					(ch >= 'A' && ch <= 'Z'))
			{
				bTag = 1;
				continue;
			}
			else
				sTmp += '<';
		}
		else if (ch == '<')
		{
			bLT = 1;
			continue;
		}

		sTmp += ch;
	}

	sStr = sTmp;
}

void CXMLParser::FinishWaypointRecord()
{
	int i;

	StripTags(m_sFields[FLD_HINTS]);
	CUtil::StripWhitespace(m_sFields[FLD_HINTS]);
	if (!m_bDecodeHints)
		EncodeHints(FLD_HINTS);

	double dLat, dLon;
	ConvertCoords(m_sFields[FLD_LAT], m_sFields[FLD_LON],
		m_sFields[FLD_COORD], dLat, dLon);

	m_sFields[FLD_DESC].erase();

	string sDesc;
	for (i=FLD_SHRT_DESC; i<= FLD_LONG_DESC; i++)
	{
		string &rDesc = m_sFields[i];

		if (IsWhitespace(rDesc))
			continue;

		CUtil::StripWhitespace(rDesc);
		if (i == FLD_SHRT_DESC)
			rDesc += "\n\n";

		sDesc += rDesc;
	}

	string &rDesc = m_sFields[FLD_DESC];
	int bExtra = 0;

	// CM2GPX-supported fields

	if (m_bOwner && !IsWhitespace(m_sFields[FLD_OWNER]))
	{
		rDesc += "Owner: ";
		rDesc += m_sFields[FLD_OWNER] + "\n";
		bExtra = 1;
	}

	if (m_bDate && !IsWhitespace(m_sFields[FLD_DATETIME]))
	{
		rDesc += "Date: ";

		string &rDT = m_sFields[FLD_DATETIME];
		int i = rDT.find('T');
		if (i != string::npos)
			rDT = rDT.substr(0, i);
		ReformatDate(rDT);

		rDesc += rDT;
		rDesc += "\n";
		bExtra = 1;
	}

	if (m_bLocation && (!IsWhitespace(m_sFields[FLD_COUNTRY]) ||
		!IsWhitespace(m_sFields[FLD_STATE]) ||
		!IsWhitespace(m_sFields[FLD_LOCALE])))
	{
		rDesc += "Location: ";

		if (!IsWhitespace(m_sFields[FLD_LOCALE]))
			rDesc += m_sFields[FLD_LOCALE] + ", ";
		if (!IsWhitespace(m_sFields[FLD_STATE]))
			rDesc += m_sFields[FLD_STATE] + ", ";

		rDesc += m_sFields[FLD_COUNTRY] + "\n";
		bExtra = 1;
	}

	if (m_bContainer && !IsWhitespace(m_sFields[FLD_CONTAINER]))
	{
		rDesc += "Container: ";
		rDesc += m_sFields[FLD_CONTAINER] + "\n";
		bExtra = 1;
	}

	if (m_bShowBugs && !IsWhitespace(m_sFields[FLD_BUGS]))
	{
		rDesc += "Bugs: ";
		rDesc += m_sFields[FLD_BUGS] + "\n";
		bExtra = 1;
	}

	if (m_bCacheStatus && !IsWhitespace(m_sFields[FLD_STATUS]))
	{
		rDesc += "Status: ";
		rDesc += m_sFields[FLD_STATUS] + "\n";
		bExtra = 1;
	}

	// New fields
	int bExtra2 = 0;

	if (!IsWhitespace(m_sFields[FLD_CAMO_DIFF]))
	{
		if (!bExtra2)
			rDesc += "\n";

		rDesc += "Camo challenge: ";
		rDesc += m_sFields[FLD_CAMO_DIFF] + "\n";
		bExtra2 = bExtra = 1;
	}

	if (bExtra)
		rDesc += "\n";

	CUtil::StripWhitespace(sDesc);
	rDesc += sDesc;

	int bDescTrunc = 0;
	if (rDesc.size() > m_nMaxDesc)
	{
		rDesc = rDesc.substr(0, m_nMaxDesc - 12);
		rDesc += "\n[truncated]";
		bDescTrunc = 1;
	}

	if (!rDesc.empty())
		m_bEmptyDesc = 0;

	if (!m_sFields[FLD_NAME2].empty())
		m_sFields[FLD_NAME] = m_sFields[FLD_NAME2];
	if (!m_sFields[FLD_TYPE2].empty())
		m_sFields[FLD_TYPE] = m_sFields[FLD_TYPE2];

	if (m_bStripNameQuotes)
		StripQuotes(m_sFields[FLD_NAME]);

	CUtil::StripWhitespace(m_sFields[FLD_NAME]);
	if (m_sFields[FLD_NAME].empty())
		m_sFields[FLD_NAME] = "Geocache";

	if (m_bLogTemplate)
		m_sFields[FLD_NOTES] = "Took: \nLeft: \nTB: \n";

	string sSym = m_sFields[FLD_SYMBOL];
	CUtil::LowercaseString(sSym);
	if (sSym.find("geocache") == string::npos)
	{
		string sType = m_sFields[FLD_TYPE];
		CUtil::LowercaseString(sType);
		if (sType.find("geocache") == string::npos)
			m_bNonCacheFile = 1;
	}

	// If GPX description is kind of long and this isn't a cache
	// file, treat it as the description instead
	if (m_bNonCacheFile && rDesc.empty() &&
			m_sFields[FLD_NAME].size() > 64)
		m_bLongDesc = 1;

	if (m_bLongDesc)
	{
		rDesc = m_sFields[FLD_NAME];
		m_sFields[FLD_NAME] = m_sFields[FLD_WAYPOINT];
	}

	string &rCmt = m_sFields[FLD_COMMENT];
	CUtil::StripWhitespace(rCmt);
	if (!rCmt.empty())
	{
		if (!rDesc.empty())
			rDesc += "\n\nGPX Comment: ";
		rDesc += rCmt;
	}

	if (m_sFields[FLD_TYPE].substr(0, 9) == "Waypoint|")
		m_sFields[FLD_TYPE] = m_sFields[FLD_TYPE].substr(9);

	char sep = 1;
	m_sRecord.erase();

	for (i=0; i<MAX_END_FIELDS; i++)
	{
		m_sRecord += m_sFields[i];
		m_sRecord += sep;
	}

	CWPData *pWP = new CWPData();
	pWP->m_sRecord = m_sRecord;
	pWP->m_sWaypoint = m_sFields[FLD_WAYPOINT];
	pWP->m_sDesc = m_sFields[FLD_NAME];
	pWP->m_sDiff = m_sFields[FLD_DIFFICULTY];
	pWP->m_sTerrain = m_sFields[FLD_TERRAIN];
	pWP->m_sSymbol = m_sFields[FLD_SYMBOL];
	pWP->m_sType = m_sFields[FLD_TYPE];
	pWP->m_sContainer = m_sFields[FLD_CONTAINER];
	pWP->m_sState = m_sFields[FLD_STATE];
	pWP->m_sCountry = m_sFields[FLD_COUNTRY];
	pWP->m_sOwner = m_sFields[FLD_OWNER];
	pWP->m_bTravelBugs = m_bHasBugs;
	pWP->m_dLat = dLat;
	pWP->m_dLon = dLon;
	pWP->m_bTruncated = bDescTrunc;
	pWP->m_sURL = m_sFields[FLD_URL];
	pWP->m_sLinks = m_sFields[FLD_LINKS];
	pWP->m_bActive = m_bCacheActive;
	m_pList->AddWP(pWP);
}

int CXMLParser::CheckCharVal(char *pVal)
{
	u_long val;
	int legal = 0;

	if (pVal[0] == 'x')
		val = strtoul(&pVal[1], NULL, 16);
	else
		val = strtoul(pVal, NULL, 10);

	if (val < 32)
	{
		if (val == 9 || val == 10 || val == 13)
			legal = 1;
	}
	else
		legal = 1;

	return legal;
}

void CXMLParser::CleanCharRefs(char *szBuf)
{
	char *pRef, *pEnd;

	pRef = strstr(szBuf, "&#");
	while (pRef)
	{
		pEnd = strchr(pRef, ';');
		if (!pEnd)
		{
			*pRef = 0;
			break;
		}

		pEnd++;

		if (!CheckCharVal(&pRef[2]))
			strcpy(pRef, pEnd);
		else
			pRef = pEnd;

		pRef = strstr(pRef, "&#");
	}
}

void CXMLParser::FormatFileTS(string sPath)
{
	struct stat info;
	struct tm *t;
	char buf[32];

	if (stat(sPath.c_str(), &info) < 0)
		return;

	t = gmtime(&info.st_mtime);
	sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ",
		t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour,
		t->tm_min, t->tm_sec);
	m_sFileTS = buf;
}

int CXMLParser::ParseFile(string sPath, IXMLReader *pReader)
{
	int bError = 0;

	FormatFileTS(sPath);

	XML_Parser pParser = XML_ParserCreate(NULL);
	if (!pParser)
	{
		printf("Couldn't allocate parser.\n");
		return 0;
	}

	XML_SetUserData(pParser, this);
	XML_SetElementHandler(pParser, CXMLParser::HandleElemStart,
		CXMLParser::HandleElemEnd);
	XML_SetCharacterDataHandler(pParser, CXMLParser::HandleCharData);

	for (;;)
	{
		int len;

		len = pReader->Read(m_buf, CHUNK_SIZE-1);
		m_buf[len] = 0;
		if (len < 0)
		{
			bError = 1;
			printf("Error reading input file.\n");
			break;
		}

		CleanCharRefs(m_buf);
		len = strlen(m_buf);
		if (!XML_Parse(pParser, m_buf, len, len == 0))
		{
			bError = 1;
			printf("Parse error at line %d:\n%s\n",
				XML_GetCurrentLineNumber(pParser),
				XML_ErrorString(XML_GetErrorCode(pParser)));
			break;
		}

		if (len == 0)
			break;
	}

	XML_ParserFree(pParser);

	if (m_bLocWarning || m_bNonCacheFile || (m_pList->m_List.size() == 0))
		m_bEmptyDesc = 0;

	return (bError != 1);
}
