/*
    Copyright 2004-2005 Brian Smith (brian@smittyware.com)
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
#include "htmlwriter.h"
#include "util.h"

CHTMLWriter::CHTMLWriter()
{
	m_fp = NULL;
}

CHTMLWriter::~CHTMLWriter()
{
}

void CHTMLWriter::SetFileName(string sPath)
{
	int nDot = sPath.rfind('.');
	if (nDot != string::npos)
	{
		string sExt = sPath.substr(nDot);
		CUtil::LowercaseString(sExt);

		if (sExt == ".pdb")
			sPath = sPath.substr(0, nDot);
	}

	sPath += "-Links.html";
	m_sPath = sPath;
}

int CHTMLWriter::WriteCacheRecord(CWPData *pRec)
{
	if (!m_fp)
	{
		m_fp = fopen(m_sPath.c_str(), "w");
		if (!m_fp)
		{
			printf("Error opening links HTML file for writing.\n");
			return 0;
		}

		fprintf(m_fp, "<html><body>\n");
	}

	int bWptURL = !pRec->m_sURL.empty();
	fprintf(m_fp, "<h3><b>%s - ", pRec->m_sWaypoint.c_str());
	if (bWptURL)
	{
		fprintf(m_fp, "<a href=\"%s\" target=\"_blank\">",
			pRec->m_sURL.c_str());
	}
	fprintf(m_fp, "%s", pRec->m_sDesc.c_str());
	if (bWptURL)
		fprintf(m_fp, "</a>");
	fprintf(m_fp, "</b></h3>\n<ul>\n");

	string sURL, sRemain = pRec->m_sLinks;
	int nIndex = sRemain.find('\001');
	while (nIndex != string::npos)
	{
		sURL = sRemain.substr(0, nIndex);
		sRemain = sRemain.substr(nIndex+1);

		fprintf(m_fp, "<li><a href=\"%s\" target=\"_blank\">",
			sURL.c_str());
		fprintf(m_fp, "%s</a></li>\n", sURL.c_str());

		nIndex = sRemain.find('\001');
	}

	fprintf(m_fp, "</ul>\n");
	return 1;
}

void CHTMLWriter::WriteFile(string sPdbPath, int bQuiet)
{
	SetFileName(sPdbPath);

	typedef vector<string> StringVec;
	StringVec wps;

	WPList::iterator iter = m_pList->m_List.begin();
	while (iter != m_pList->m_List.end())
	{
		CWPData *pRec = *iter;
		if (pRec->m_bConvert && !pRec->m_sLinks.empty())
			wps.push_back(pRec->m_sWaypoint);

		iter++;
	}

	sort(wps.begin(), wps.end());

	StringVec::iterator iter2 = wps.begin();
	while (iter2 != wps.end())
	{
		string sCur = *iter2;

		iter = m_pList->m_List.begin();
		while (iter != m_pList->m_List.end())
		{
			CWPData *pRec = *iter;
			if (pRec->m_sWaypoint == sCur)
			{
				if (!WriteCacheRecord(pRec))
					return;

				break;
			}

			iter++;
		}

		iter2++;
	}

	if (m_fp)
	{
		fprintf(m_fp, "</body></html>\n");
		fclose(m_fp);
		m_fp = NULL;

		if (!bQuiet)
		{
			printf("Cache links written to %s\n",
				m_sPath.c_str());
		}
	}
}
