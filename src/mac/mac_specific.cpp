/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
int stricmp( const char *s1, const char *s2 ) {
	char c1, c2;
	while ( 1 )
	{
		c1 = tolower( *s1++ );
		c2 = tolower( *s2++ );
		if ( c1 < c2 ) {
			return -1;
		}
		if ( c1 > c2 ) {
			return 1;
		}
		if ( c1 == 0 ) {
			return 0;
		}
	}
}
int strnicmp( const char *s1, const char *s2, unsigned long n ) {
	int i;
	char c1, c2;
	for ( i = 0; i < n; i++ )
	{
		c1 = tolower( *s1++ );
		c2 = tolower( *s2++ );
		if ( c1 < c2 ) {
			return -1;
		}
		if ( c1 > c2 ) {
			return 1;
		}
		if ( !c1 ) {
			return 0;
		}
	}
	return 0;
}

char* strlwr( char *string ) {
	char *s = string;

	while ( *s )
	{
		*s = tolower( *s );
		s++;
	}

	return string;
}

char* strupr( char *string ) {
	char *s = string;

	while ( *s )
	{
		*s = toupper( *s );
		s++;
	}

	return string;
}

char* strrev( char *str ) {
	int SmallIndex = 0;
	int BigIndex = strlen( str ) - 1;

	while ( SmallIndex < BigIndex ) {
		char Temp = str[SmallIndex];

		str[SmallIndex] = str[BigIndex];
		str[BigIndex] = Temp;

		SmallIndex++;
		BigIndex--;
	}

	return str;
}

char* itoa( int val, char *str, int radix ) {
	char IsNegative = 0;
	int theNum = val;
	int StrIndex = 0;

	if ( theNum < 0 ) {
		theNum = -theNum;
		IsNegative = 1;
	}

	do {
		int CurDigit = theNum % radix;
		if ( CurDigit > 9 ) {
			str[StrIndex++] = CurDigit + 'A' - 10;
		} else {
			str[StrIndex++] = CurDigit + '0';
		}

		theNum /= radix;
	} while ( theNum );

	if ( IsNegative ) {
		str[StrIndex++] = '-';
	}
	str[StrIndex++] = 0;

	// Now reverse the string.
#ifdef __MACH__ // jcb, 11/10/03
	strrev( str );
#else
	_strrev( str );
#endif

	return str;
}

#ifdef __cplusplus
}
#endif

//extern "C" { extern void DebugStr (unsigned char *); }
//extern void	Sys_Print( const char *text );
void OutputDebugString( const char * s ) {
#ifdef _DEBUG
//	Sys_Print (s);
//	DebugStr((unsigned char *)s);
#endif
}
