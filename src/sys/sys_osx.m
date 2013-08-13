/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 *
 * @file sys_osx.m
 * @brief This file is just some Mac-specific bits.
 * Most of the Mac OS X code is shared with other Unix platforms in sys_unix.c
 */

#ifndef __APPLE__
#error This file is for Mac OS X only. You probably should not compile it.
#endif

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>


/**
 * @brief Display an OS X dialog box
 */
dialogResult_t Sys_Dialog(dialogType_t type, const char *message, const char *title)
{
	dialogResult_t result = DR_OK;
	NSAlert        *alert = [NSAlert new];

	[alert setMessageText: [NSString stringWithUTF8String: title]];
	[alert setInformativeText: [NSString stringWithUTF8String: message]];

	if (type == DT_ERROR)
	{
		[alert setAlertStyle: NSCriticalAlertStyle];
	}
	else
	{
		[alert setAlertStyle: NSWarningAlertStyle];
	}

	switch (type)
	{
	default:
		[alert runModal];
		result = DR_OK;
		break;

	case DT_YES_NO:
		[alert addButtonWithTitle: @"Yes"];
		[alert addButtonWithTitle: @"No"];
		switch ([alert runModal])
		{
		default:
		case NSAlertFirstButtonReturn: result  = DR_YES; break;
		case NSAlertSecondButtonReturn: result = DR_NO; break;
		}
		break;

	case DT_OK_CANCEL:
		[alert addButtonWithTitle: @"OK"];
		[alert addButtonWithTitle: @"Cancel"];

		switch ([alert runModal])
		{
		default:
		case NSAlertFirstButtonReturn: result  = DR_OK; break;
		case NSAlertSecondButtonReturn: result = DR_CANCEL; break;
		}
		break;
	}

	[alert release];

	return result;
}
