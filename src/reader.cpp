/*
    Copyright 2004 Brian Smith (brian@smittyware.com)
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
#include "reader.h"

// Normal (unzipped) file reader

int CXMLReader::Open(const char *szFile)
{
	int fd = open(szFile, O_RDONLY);

	if (fd >= 0)
	{
		m_fp = fd;
		return 1;	
	}

	return 0;
}

int CXMLReader::Read(char *pBuf, int nLen)
{
	return read(m_fp, pBuf, nLen);
}

void CXMLReader::Close()
{
	close(m_fp);
}

#if HAVE_LIBZ && HAVE_LIBZZIP
// Zipped file reader

// Open zip file and look for first GPX file (if any)
int CZIPReader::Open(const char *szName)
{
	m_pDir = zzip_dir_open(szName, 0);
	if (!m_pDir)
		return 0;

	int bFound = 0;
	ZZIP_DIRENT dirent;
	while (zzip_dir_read(m_pDir, &dirent))
	{
		string sName = dirent.d_name;
		if (sName.size() < 5)
			continue;

		string sExt = sName.substr(sName.size() - 4);
		if (sExt != ".gpx" && sExt != ".GPX")
			continue;

		if (dirent.st_size == 0)
			continue;

		bFound = 1;
		break;
	}

	if (!bFound)
	{
		printf("Couldn't find GPX file in %s\n", szName);
		zzip_dir_close(m_pDir);
		return 0;
	}

	if (!m_bQuiet)
		printf("Found \"%s\" in ZIP file: %s\n", dirent.d_name, szName);

	m_pFile = zzip_file_open(m_pDir, dirent.d_name, 0);
	if (!m_pFile)
	{
		zzip_dir_close(m_pDir);
		return 0;
	}

	return 1;
}

int CZIPReader::Read(char *pBuf, int nLen)
{
	return zzip_file_read(m_pFile, pBuf, nLen);
}

void CZIPReader::Close()
{
	zzip_file_close(m_pFile);
	zzip_dir_close(m_pDir);
}

#endif // HAVE_LIBZ && HAVE_LIBZZIP
