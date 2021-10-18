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

#ifndef _PDB_H_INCLUDED_
#define _PDB_H_INCLUDED_

typedef uint8_t UInt8;
typedef uint16_t UInt16;
typedef uint32_t UInt32;

typedef UInt32 LocalID;

#ifdef __GNUC__
# define PACKED __attribute__ ((__packed__))
#else
# define PACKED
# ifdef WIN32_BUILD
#  pragma pack(push, 1)
# else
#  pragma pack 1
# endif
#endif

typedef struct PACKED _PDBRecordList
{
	LocalID nextRecordListID;
	UInt16 numRecords;
	UInt16 dummy;
} PDBRecordList;

typedef struct PACKED
{
	UInt8 name[32];
	UInt16 attributes;
	UInt16 version;
	UInt32 creationDate;
	UInt32 modificationDate;
	UInt32 lastBackupDate;
	UInt32 modificationNumber;
	LocalID appInfoID;
	LocalID sortInfoID;
	UInt32 type;
	UInt32 creator;
	UInt32 uniqueIDSeed;
	PDBRecordList recordList;
} PDBHeader;

typedef struct PACKED
{
	LocalID localChunkID;
	UInt8 attributes;
	UInt8 uniqueID[3];
} PDBRecordEntry;

#ifndef __GNUC__
# if WIN32_BUILD
#  pragma pack(pop)
# else
#  pragma pack
# endif
#endif

// Creator/type IDs
#define PDB_CREATOR "cMat"
#define PDB_TYPE "Impt"

// This is to correct the difference between time() and TimGetSeconds()
#define TIME_OFS 2082844800ul

#endif // _PDB_H_INCLUDED_
