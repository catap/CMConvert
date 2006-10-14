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
# define PACKED __attribute__ ((__packed__));
#else
# define PACKED
# ifdef WIN32_BUILD
#  pragma pack(push, 1)
# else
#  pragma pack 1
# endif
#endif

typedef struct _PDBRecordList
{
	LocalID nextRecordListID PACKED;
	UInt16 numRecords PACKED;
	UInt16 dummy PACKED;
} PDBRecordList;

typedef struct
{
	UInt8 name[32] PACKED;
	UInt16 attributes PACKED;
	UInt16 version PACKED;
	UInt32 creationDate PACKED;
	UInt32 modificationDate PACKED;
	UInt32 lastBackupDate PACKED;
	UInt32 modificationNumber PACKED;
	LocalID appInfoID PACKED;
	LocalID sortInfoID PACKED;
	UInt32 type PACKED;
	UInt32 creator PACKED;
	UInt32 uniqueIDSeed PACKED;
	PDBRecordList recordList PACKED;
} PDBHeader;

typedef struct
{
	LocalID localChunkID PACKED;
	UInt8 attributes PACKED;
	UInt8 uniqueID[3] PACKED;
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
