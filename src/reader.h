/*
    Copyright 2004-2006 Brian Smith (brian@smittyware.com)
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

#ifndef _READER_H_INCLUDED_
#define _READER_H_INCLUDED_

// Base virtual (interface) class
class IXMLReader
{
public:
	virtual int Open(const char *szFile) = 0;
	virtual int Read(char *pBuf, int nLen) = 0;
	virtual int NextFile() = 0;
	virtual void Close() = 0;

	virtual ~IXMLReader() {

	}

	int m_bQuiet;
};

class CXMLReader : public IXMLReader
{
public:
	virtual int Open(const char *szFile);
	virtual int Read(char *pBuf, int nLen);
	virtual int NextFile();
	virtual void Close();

private:
	int m_fp;
};

#if HAVE_LIBZ && HAVE_LIBZZIP
extern "C" {
#if HAVE_ZZIP_LIB_H
# include <zzip/lib.h>
#else
# include <zzip.h>
#endif
}

class CZIPReader : public IXMLReader
{
public:
	virtual int Open(const char *szFile);
	virtual int Read(char *pBuf, int nLen);
	virtual int NextFile();
	virtual void Close();

private:
	ZZIP_DIR *m_pDir;
	ZZIP_FILE *m_pFile;
	string m_sFile;
};
#endif

#endif // _READER_H_INCLUDED_
