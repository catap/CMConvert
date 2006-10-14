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
#include "util.h"

void CUtil::LowercaseString(string &sStr)
{
        int n = sStr.size();
        
        while (n--)
        {
                char ch = sStr[n];
                if (ch >= 'A' && ch <= 'Z')
                        sStr[n] = ch - 'A' + 'a';
        }
}

void CUtil::StripWhitespace(string &rStr)
{
        while (strchr(" \t\r\n", rStr[0]) != NULL && rStr.size() > 0)
                rStr = rStr.substr(1);

        int nLen = rStr.size();
        while (nLen > 0 && strchr(" \t\r\n", rStr[nLen-1]) != NULL)
                rStr = rStr.substr(0, --nLen);
}
