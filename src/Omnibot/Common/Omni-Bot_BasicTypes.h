////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy: DrEvil $
// $LastChangedDate: 2005-09-20 06:39:17 -0700 (Tue, 20 Sep 2005) $
// $LastChangedRevision: 973 $
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __OMNIBOT_BASICTYPES_WIN32_H__
#define __OMNIBOT_BASICTYPES_WIN32_H__

// Basic Variable Types.

// Windows platforms.
#ifdef _MSC_VER

	typedef char						obint8;
	typedef unsigned char				obuint8;
	typedef short						obint16;
	typedef unsigned short				obuint16;
	typedef int							obint32;
	typedef unsigned int				obuint32;
	typedef __int64						obint64;
	typedef unsigned __int64			obuint64;
	typedef float						obReal;
	typedef void*						obvoidp;

#else

	typedef char						obint8;
	typedef unsigned char				obuint8;
	typedef short						obint16;
	typedef unsigned short				obuint16;
	typedef int							obint32;
	typedef unsigned int				obuint32;
	typedef long long int				obint64;
	typedef unsigned long long int		obuint64;
	typedef float						obReal;
	typedef void*						obvoidp;

#endif

#endif
