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

#ifndef _PDBWRITER_H_INCLUDED_
#define _PDBWRITER_H_INCLUDED_

#include "pdb.h"
#include "wplist.h"

#include <list>
typedef list<string> StringList;

class CPDBWriter
{
public:
	CPDBWriter();
	~CPDBWriter();

	CWPList *m_pList;

	int BuildHeader(string sPath);
	int WriteFile(string sPath, int bQuiet);

private:
	PDBHeader *m_pHeader;
	u_long m_nHeaderSize;

	int ConvertCount();
	string GetBaseName(string sPath);
};

#endif // _PDBWRITER_H_INCLUDED_
