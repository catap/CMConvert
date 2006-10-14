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
#include "pdbwriter.h"
#include "util.h"

static UInt32 pdbLongSwap(UInt32 val)
{
#ifdef WORDS_BIGENDIAN
	return val;
#else
	UInt32 out;
	UInt8 *pIn = (UInt8*)&val, *pOut = (UInt8*)&out;
	int i;

	for (i=0; i<4; i++)
		pOut[3-i] = pIn[i];

	return out;
#endif
}

static UInt16 pdbShortSwap(UInt16 val)
{
#ifdef WORDS_BIGENDIAN
	return val;
#else
	UInt16 out;
	UInt8 *pIn = (UInt8*)&val, *pOut = (UInt8*)&out;
	int i;

	for (i=0; i<2; i++)
		pOut[1-i] = pIn[i];

	return out;
#endif
}

CPDBWriter::CPDBWriter()
{
	m_pHeader = NULL;
}

CPDBWriter::~CPDBWriter()
{
	if (m_pHeader)
		free(m_pHeader);
}

int CPDBWriter::ConvertCount()
{
	int count = 0;
	WPList::iterator iter = m_pList->m_List.begin();
	while (iter != m_pList->m_List.end())
	{
		if ((*iter)->m_bConvert)
			count++;

		iter++;
	}

	return count;
}

string CPDBWriter::GetBaseName(string sPath)
{
	int nSlash, nDot;
	string sBase = sPath;

	nSlash = sBase.rfind(PATH_SEP);
	if (nSlash != string::npos)
		sBase = sBase.substr(nSlash+1);

	nDot = sBase.rfind('.');
	if (nDot != string::npos)
	{
		string sExt = sBase.substr(nDot);
		CUtil::LowercaseString(sExt);

		if (sExt == ".pdb")
			sBase = sBase.substr(0, nDot);
	}

	return sBase;
}

int CPDBWriter::BuildHeader(string sPath)
{
	int count = ConvertCount();
	if (count == 0)
	{
		printf("No waypoints to convert.\n");
		return 0;
	}

	u_long len = sizeof(PDBHeader) + count * sizeof(PDBRecordEntry);
	time_t ct = time(NULL);
	u_long ofs = len;

	m_pHeader = (PDBHeader*)malloc(len);
	if (!m_pHeader)
	{
		printf("Couldn't build PDB header.\n");
		return 0;
	}

#if HAVE_MEMSET
	memset(m_pHeader, 0, len);
#else
	bzero(m_pHeader, len);
#endif

	string sDBName = "cMat-";
	sDBName += GetBaseName(sPath);
	if (sDBName.size() > 31)
		sDBName = sDBName.substr(0, 31);

	strcpy((char*)m_pHeader->name, sDBName.c_str());
	m_pHeader->version = pdbShortSwap(1);
	m_pHeader->creationDate = pdbLongSwap((UInt32)ct + (UInt32)TIME_OFS);
	m_pHeader->modificationDate = m_pHeader->creationDate;
	m_pHeader->modificationNumber = pdbLongSwap(1);
	m_pHeader->attributes = pdbShortSwap(0);
	memcpy(&m_pHeader->type, PDB_TYPE, 4);
	memcpy(&m_pHeader->creator, PDB_CREATOR, 4);
	m_pHeader->recordList.numRecords = pdbShortSwap(count);

	PDBRecordEntry *pRec = (PDBRecordEntry*)(((char*)m_pHeader) + 
		sizeof(PDBHeader) - 2);

	WPList::iterator iter = m_pList->m_List.begin();
	while (iter != m_pList->m_List.end())
	{
		if ((*iter)->m_bConvert)
		{
			pRec->localChunkID = pdbLongSwap(ofs);
			ofs += (*iter)->m_sRecord.size() + 1;
			pRec++;
		}

		iter++;
	}

	m_nHeaderSize = len;

	return 1;
}

int CPDBWriter::WriteFile(string sPath, int bQuiet)
{
	int fd;
	int count = 0;

	fd = open(sPath.c_str(), PDB_OPEN_FLAGS, 0600);
	if (fd >= 0)
	{
		write(fd, m_pHeader, m_nHeaderSize);

		WPList::iterator iter = m_pList->m_List.begin();
		while (iter != m_pList->m_List.end())
		{
			if ((*iter)->m_bConvert)
			{
				write(fd, (*iter)->m_sRecord.c_str(),
					(*iter)->m_sRecord.size() + 1);

				count++;
			}

			iter++;
		}

		close(fd);

		if (!bQuiet)
		{
			printf("%d waypoint%s converted.\n", count,
				(count == 1) ? "" : "s");
		}
		return 1;
	}

	return 0;
}

