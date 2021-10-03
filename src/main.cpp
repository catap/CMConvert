/*
    Copyright 2003-2010 Brian Smith (brian@smittyware.com)
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
#include "wplist.h"
#include "pdbwriter.h"
#include "getopt.h"
#include "htmlwriter.h"
#include "util.h"
#include "reader.h"

#ifdef HAVE_LIBM
#include <math.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <list>

typedef list<string> StringList;

static string sOutputPath;
static string sInputPath;
static StringList slWaypoints;
static int bListWP, bOwner, bContainer, bDate, bLocation, bShowBugs;
static int bSymFound, bSymNotFound, bDecodeHints, bFilterBugs, bShowVer;
static int bQuietMode, nMaxLogs, nMaxDesc, bLogTemplate, bWriteHTML;
static int bCacheStatus, bFiltActive, bFiltInactive, bUseTS;
static int bLocWarned, bEmptyWarned, bStripQuotes;
static string sStateFilt, sCountryFilt, sOwnerFilt, sTypeFilt, sSymFilt,
	sContFilt, sRadiusFilt, sExcludeFilt;

// String filter options...
static struct option long_options[] = {
	{ "state", 1, 0, 0 },
	{ "country", 1, 0, 0 },
	{ "cont", 1, 0, 0 },
	{ "type", 1, 0, 0 },
	{ "owner", 1, 0, 0 },
	{ "sym", 1, 0, 0 },
	{ "excl", 1, 0, 0 },
	{ "filter", 1, 0, 0 },
#ifdef HAVE_LIBM
	{ "radius", 1, 0, 0 },
#endif
	{ 0, 0, 0, 0 }
};
static string* filter_str[] = { &sStateFilt, &sCountryFilt, &sContFilt, 
	&sTypeFilt, &sOwnerFilt, &sSymFilt, &sExcludeFilt, NULL,
#ifdef HAVE_LIBM
	&sRadiusFilt
#endif
};

string GetDefaultOutputFile(string sPath)
{
	int nSlash, nDot, nComma;
	string sOutPath;

	nComma = sPath.rfind(',');
	if (nComma != string::npos)
		sPath = sPath.substr(0, nComma);

	nDot = sPath.rfind('.');
	nSlash = sPath.rfind(PATH_SEP);

	if (nDot == string::npos)
		sOutPath = sPath;
	else if (nSlash == string::npos || nSlash < nDot)
		sOutPath = sPath.substr(0, nDot);
	else
		sOutPath = sPath;

	return (sOutPath + ".pdb");
}

void AddFilterString(int nIndex, string sStr)
{
	bool bRad = (filter_str[nIndex] == &sRadiusFilt);

	string &rStr = *(filter_str[nIndex]);
	if (!rStr.empty())
	{
		if (bRad)
			rStr.erase();
		else
			rStr += ":";
	}

	rStr += sStr;
}

int ReadFilterFile(string sFile)
{
	int bSuccess = 0;

	FILE *fp = fopen(sFile.c_str(), "r");
	if (fp)
	{
		int nCount = 0;
		char buf[512];
		while (fgets(buf, 512, fp))
		{
			string sLine, sName;
			sLine = buf;

			int nIndex = sLine.find('=');
			if (nIndex == string::npos)
				continue;
			sName = sLine.substr(0, nIndex);
			sLine = sLine.substr(nIndex+1);

			CUtil::StripWhitespace(sName);
			CUtil::StripWhitespace(sLine);
			CUtil::LowercaseString(sName);

			nIndex = 0;
			struct option *pOpt = long_options;
			while (pOpt->name)
			{
				if (sName == pOpt->name)
					break;

				pOpt++;
				nIndex++;
			}

			if (!pOpt->name)
				continue; // No valid option found
			if (nIndex == 7)
				continue; // Recursion?  Hell no...

			AddFilterString(nIndex, sLine);
			nCount++;
		}

		if (!bQuietMode)
		{
			printf("%d valid filter%s read from: %s\n", nCount, 
				(nCount == 1) ? "" : "s", sFile.c_str());
		}

		fclose(fp);
		bSuccess = 1;
	}
	else
		printf("Can't open filter file: %s\n", sFile.c_str());

	return bSuccess;
}

int ParseCommandLine(int argc, char **argv)
{
	int errflg = 0;
	extern int optind;
	extern char *optarg;
	int c;

	bListWP = 0;
	bDate = 0;
	bOwner = 0;
	bContainer = 0;
	bLocation = 0;
	bShowBugs = 0;
	nMaxLogs = 0;
	nMaxDesc = 8192;
	bSymFound = 0;
	bSymNotFound = 0;
	bDecodeHints = 0;
	bFilterBugs = 0;
	bShowVer = 0;
	bQuietMode = 0;
	bLogTemplate = 0;
	bWriteHTML = 0;
	bCacheStatus = 0;
	bFiltActive = 0;
	bFiltInactive = 0;
	bStripQuotes = 0;
	bUseTS = 1;

	slWaypoints.clear();

	sInputPath.erase();
	sOutputPath.erase();

	sContFilt.erase();
	sCountryFilt.erase();
	sStateFilt.erase();
	sTypeFilt.erase();
	sOwnerFilt.erase();
	sSymFilt.erase();
	sRadiusFilt.erase();
	sExcludeFilt.erase();

	if (argc < 2)
		return 0;

	while (1)
	{
		int option_index = 0;

		c = getopt_long(argc, argv, "aAbBCdDfFhHlLN:o:OqsStTv",
			long_options, &option_index);
		if (c == -1)
			break;

		switch (c)
		{
		case 0:
			if (option_index == 7)
			{
				if (!ReadFilterFile(optarg))
					errflg = 1;
			}
			else
				AddFilterString(option_index, optarg);
			break;
		case 'a':	bFiltActive = 1; break;
		case 'A':	bFiltInactive = 1; break;
		case 'b':	bFilterBugs = 1; break;
		case 'B':	bShowBugs = 1; break;
		case 'C':	bContainer = 1; break;
		case 'D':	bDate = 1; break;
		case 'd':	break;
		case 'f':	bSymFound = 1; break;
		case 'F':	bSymNotFound = 1; break;
		case 'h':	bWriteHTML = 1; break;
		case 'H':	bDecodeHints = 1; break;
		case 'L':	bLocation = 1; break;
		case 'l':	bListWP = 1; break;
		case 'N':
			{
				char *end;
				nMaxLogs = strtol(optarg, &end, 10);
				if (*end != 0)
					errflg = 1;
				break;
			}
		case 'O':	bOwner = 1; break;
		case 'o':	sOutputPath = optarg; break;
		case 'q':	bQuietMode = 1; break;
		case 's':	bStripQuotes = 1; break;
		case 'S':	bCacheStatus = 1; break;
		case 'T':	bUseTS = 0; break;
		case 't':	bLogTemplate = 1; break;
		case 'v':	bShowVer = 1; break;
		case '?':	errflg = 1; break;
		}
	}

	if (bShowVer)
		return 1;
        if ((optind >= argc) || errflg)
                return 0;

	sInputPath = argv[optind];

	int i = optind + 1;
	while (i < argc)
	{
		slWaypoints.push_back(argv[i]);
		i++;
	}

	return 1;
}

int StringInList(StringList &rList, string &rStr)
{
	StringList::iterator iter = rList.begin();

	while (iter != rList.end())
	{
		if (*iter == rStr)
			return 1;

		iter++;
	}

	return 0;
}

int PrintVersion()
{
	printf(PACKAGE_STRING
#if HAVE_LIBZ && HAVE_LIBZZIP
		"+zip"
#endif
		" -- Copyright (C) 2003-2010 Brian Smith\n");
	return 0;
}

int PrintUsage(char *szExe)
{
        printf("Usage: %s [-a] [-A] [-b] [-B] [-C] [-D] [-f] [-F] [-h] [-H]\n"
	"\t[-l] [-L] [-N max_log_count] [-o output_file] [-O] [-q] [-s] [-S]\n"
	"\t[-t] [-T] [-v] [--cont=container] [--country=country] [--sym=symbol]\n" 
	"\t[--state=state] [--owner=cache_owner] [--type=cache_type]\n"
#ifdef HAVE_LIBM
	"\t[--radius=distance,lat,lon] [--radius=distance,waypoint]\n"
#endif
	"\t[--excl=waypoint_list] [--filter=file] input_file1[,input_file2...]\n"
	"\t[waypoint ...]\n",
		szExe);

	return 2;
}

int CheckPDBStructSizes()
{
	int err = 0;

	if (sizeof(PDBHeader) != 80)
		err = 1;
	if (sizeof(PDBRecordEntry) != 8)
		err = 1;

	if (err)
	{
		printf("PDB structure sizes incorrect!  "
			"Conversion aborted.\n");
		printf("Header size = %zu (must be 80)\n",
			sizeof(PDBHeader));
		printf("Record entry size = %zu (must be 8)\n",
			sizeof(PDBRecordEntry));
	}

	return !err;
}

int check_filter_string(string sFilter, string sCheck)
{
	if (sFilter.empty())
		return 1;
	if (sCheck.empty())
		return 0;

	CUtil::LowercaseString(sCheck);
	CUtil::LowercaseString(sFilter);

	int nPos = sFilter.find(':');
	while (nPos != string::npos)
	{
		string sSub = sFilter.substr(0, nPos);
		sFilter = sFilter.substr(nPos+1);

		if (sSub.empty())
			return 0;
		if (strstr(sCheck.c_str(), sSub.c_str()))
			return 1;

		nPos = sFilter.find(':');
	}

	if (sFilter.empty())
		return 0;
	else if (strstr(sCheck.c_str(), sFilter.c_str()))
		return 1;
	else
		return 0;
}

#ifdef HAVE_LIBM
int parse_radius_filter(double &dLat, double &dLon, double &dDist,
	CWPList &rList)
{
	int nIndex;
	string sPiece, sLeft, sUnit;
	char *szUnit;

	nIndex = sRadiusFilt.find(',');
	if (nIndex == string::npos)
		return 0;
	sPiece = sRadiusFilt.substr(0, nIndex);
	sLeft = sRadiusFilt.substr(nIndex+1);
	if (sPiece.empty())
		return 0;
	dDist = strtod(sPiece.c_str(), &szUnit);
	sUnit = szUnit;
	CUtil::LowercaseString(sUnit);

	nIndex = sLeft.find(',');
	if (nIndex == string::npos)
	{
		CWPData *pWP = rList.GetByWP(sLeft);
		if (!pWP)
		{
			printf("Unknown waypoint ID - %s\n", sLeft.c_str());
			return 0;
		}

		dLat = pWP->m_dLat;
		dLon = pWP->m_dLon;

		if (!bQuietMode)
		{
			printf("Filtering by distance from %s "
				"(%.5f, %.5f)\n", sLeft.c_str(), dLat, 
				dLon);
		}
	}
	else
	{
		sPiece = sLeft.substr(0, nIndex);
		sLeft = sLeft.substr(nIndex+1);
		if (sPiece.empty() || sLeft.empty())
			return 0;
		dLat = atof(sPiece.c_str());
		dLon = atof(sLeft.c_str());
	}

	if (sUnit == "k" || sUnit == "km")
		dDist /= 6378.1370;	// Kilometers
	else if (sUnit == "mi")
		dDist /= 3963.37433180;	// Statute miles
	else
		dDist /= 3441.6427252;	// Nautical miles

	if (dDist < 0.0 || dLat < -90.0 || dLat > 90.0 || dLon < -180.0 ||
			dLon >= 180.0)
		return 0;	// Values out of range

	return 1;
}

int check_radius_filter(double dLat1, double dLon1, double dLat2,
	double dLon2, double dDist)
{
	double ddLat, ddLon, a, c;

	dLat1 *= 0.017453293;
	dLon1 *= 0.017453293;
	dLat2 *= 0.017453293;
	dLon2 *= 0.017453293;

	ddLat = dLat2 - dLat1;
	ddLon = dLon2 - dLon1;

	if (ddLat == 0.0 && ddLon == 0.0)
		return 1;

	a = pow(sin(ddLat/2), 2) + cos(dLat1) * cos(dLat2) *
		pow(sin(ddLon/2), 2);
	c = 2 * atan2(sqrt(a), sqrt(1-a));

	return (c <= dDist);
}
#endif

int parse_xml_file(string sFile, CWPList *pList)
{
	IXMLReader *pReader;

#if HAVE_LIBZ && HAVE_LIBZZIP
	string sExt = sFile.substr(sFile.size() - 4);
	CUtil::LowercaseString(sExt);
	if (sExt == ".zip")
		pReader = new CZIPReader;
	else
#endif
		pReader = new CXMLReader;

	pReader->m_bQuiet = bQuietMode;
	if (!pReader->Open(sFile.c_str()))
	{
		printf("Couldn't open file: %s\n", sFile.c_str());
		return 0;
	}

	do
	{
		CXMLParser parser;
		CWPList wplist;
		parser.m_pList = &wplist;
		parser.m_bContainer = bContainer;
		parser.m_bLocation = bLocation;
		parser.m_bOwner = bOwner;
		parser.m_bDate = bDate;
		parser.m_bShowBugs = bShowBugs;
		parser.m_bDecodeHints = bDecodeHints;
		parser.m_nMaxLogs = nMaxLogs;
		parser.m_nMaxDesc = nMaxDesc;
		parser.m_bLogTemplate = bLogTemplate;
		parser.m_bQuiet = bQuietMode;
		parser.m_bCacheStatus = bCacheStatus;
		parser.m_bStripNameQuotes = bStripQuotes;
		if (!parser.ParseFile(sFile, pReader))
			continue;

		if (!bQuietMode && parser.m_bLocWarning && !bLocWarned)
		{
			printf(
		"You are converting one or more Geocaching.com LOC files.  Please be\n"
		"aware that these files only contain the most basic information about\n"
		"caches.  See the CacheMate FAQ for more details.\n");
			bLocWarned = 1;
		}

		if (!bQuietMode && parser.m_bEmptyDesc && !bEmptyWarned)
		{
			printf(
		"WARNING:  This looks like a geocache GPX file, but is missing\n"
		"cache descriptions and other info.  If this file was originally a\n"
		"Geocaching.com pocket query, try using the original file.\n");
			bEmptyWarned = 1;
		}

		string ts = bUseTS ? parser.m_sFileTS : "";

		pList->AddList(&wplist, ts);
	} while (pReader->NextFile());

	pReader->Close();
	delete pReader;

	return 1;
}

int main(int argc, char **argv)
{
#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)
	setlocale(LC_TIME, "");
#endif

	if (!CheckPDBStructSizes())
		return 1;

	// Parse command line

	if (!ParseCommandLine(argc, argv))
	{
		PrintVersion();
		return PrintUsage(argv[0]);
	}
	if (bShowVer)
		return PrintVersion();

	if (sOutputPath.empty())
		sOutputPath = GetDefaultOutputFile(sInputPath);

	// Parse input file(s)

	bLocWarned = 0;

	CWPList wplist;
	int nComma = sInputPath.find(',');
	while (nComma != string::npos)
	{
		string sFile = sInputPath.substr(0, nComma);
		sInputPath = sInputPath.substr(nComma+1);

		if (!sFile.empty())
		{
			if (!parse_xml_file(sFile, &wplist))
				return 1;
		}

		nComma = sInputPath.find(',');
	}

	if (!sInputPath.empty())
	{
		if (!parse_xml_file(sInputPath, &wplist))
			return 1;
	}

	// Apply filters to waypoint records

#ifdef HAVE_LIBM
	double dLat, dLon, dDist;
	int bRadius = 0;

	if (!sRadiusFilt.empty())
	{
		bRadius = parse_radius_filter(dLat, dLon, dDist, wplist);
		if (!bRadius)
		{
			printf("Invalid radius filter specification.\n");
			return 2;
		}
	}
#endif

	int bAll = (slWaypoints.size() == 0);
	WPList::iterator iter = wplist.m_List.begin();
	while (iter != wplist.m_List.end())
	{
		CWPData *pData = (*iter);
		
		if (bAll || StringInList(slWaypoints, pData->m_sWaypoint))
			pData->m_bConvert = 1;

		// Travel bug filter
		if (bFilterBugs && !pData->m_bTravelBugs)
			pData->m_bConvert = 0;

		// Found/not found filter
		if (bSymFound && (pData->m_sSymbol != "Geocache Found"))
			pData->m_bConvert = 0;
		if (bSymNotFound && (pData->m_sSymbol != "Geocache"))
			pData->m_bConvert = 0;

		// Cache active/inactive filter
		if (bFiltActive && !pData->m_bActive)
			pData->m_bConvert = 0;
		if (bFiltInactive && pData->m_bActive)
			pData->m_bConvert = 0;

		// String filters
		if (!check_filter_string(sContFilt, pData->m_sContainer))
			pData->m_bConvert = 0;
		if (!check_filter_string(sCountryFilt, pData->m_sCountry))
			pData->m_bConvert = 0;
		if (!check_filter_string(sStateFilt, pData->m_sState))
			pData->m_bConvert = 0;
		if (!check_filter_string(sTypeFilt, pData->m_sType))
			pData->m_bConvert = 0;
		if (!check_filter_string(sSymFilt, pData->m_sSymbol))
			pData->m_bConvert = 0;
		if (!check_filter_string(sOwnerFilt, pData->m_sOwner))
			pData->m_bConvert = 0;

		if (!sExcludeFilt.empty())
		{
			if (check_filter_string(sExcludeFilt,
					pData->m_sWaypoint))
				pData->m_bConvert = 0;
		}

#ifdef HAVE_LIBM
		if (bRadius)
		{
			if (!check_radius_filter(dLat, dLon, pData->m_dLat,
					pData->m_dLon, dDist))
				pData->m_bConvert = 0;
		}
#endif

		iter++;
	}

	if (!bQuietMode)
	{	// Check and report any truncated descriptions
		int bWarned = 0;

		iter = wplist.m_List.begin();
		while (iter != wplist.m_List.end())
		{
			CWPData *pRec = *iter;
			iter++;

			if (!pRec->m_bTruncated || !pRec->m_bConvert)
				continue;

			if (!bWarned)
			{
				printf("Descriptions were truncated "
					"for the following records:\n");
				bWarned = 1;
			}

			printf("  %s (%s)\n", pRec->m_sDesc.c_str(),
				pRec->m_sWaypoint.c_str());
		}
	}

	if (bListWP)
	{	// Instead of converting, print a list
		printf("WP       Diff Terr TB Name\n");

		WPList::iterator iter = wplist.m_List.begin();
		while (iter != wplist.m_List.end())
		{
			if ((*iter)->m_bConvert)
			{
				printf("%-8s %-3s  %-3s  %c  %s\n",
					(*iter)->m_sWaypoint.c_str(),
					(*iter)->m_sDiff.c_str(),
					(*iter)->m_sTerrain.c_str(),
					(*iter)->m_bTravelBugs ? '*' : ' ',
					(*iter)->m_sDesc.c_str());
			}

			iter++;
		}

		return 0;
	}

	// Write the PDB file

	CPDBWriter writer;
	writer.m_pList = &wplist;
	if (!writer.BuildHeader(sOutputPath))
		return 1;

	if (!writer.WriteFile(sOutputPath, bQuietMode))
	{
		printf("Error writing output file: %s\n",
			sOutputPath.c_str());
		return 1;
	}
	else if (bWriteHTML)
	{
		CHTMLWriter writer2;
		writer2.m_pList = &wplist;
		writer2.WriteFile(sOutputPath, bQuietMode);
	}

	return 0;
}
