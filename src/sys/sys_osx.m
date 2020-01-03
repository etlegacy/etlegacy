/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
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
 */
/**
 * @file sys_osx.m
 * @brief This file is just some Mac-specific bits.
 *
 * Most of the Mac OS X code is shared with other Unix platforms in sys_unix.c
 */

#ifndef __APPLE__
#error This file is for Mac OS X only. You probably should not compile it.
#endif

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#include <dlfcn.h> // dlopen, dlclose
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


/**
 * @brief Returns the path to the user's Application Support folder.
 */
const char *OSX_ApplicationSupportPath()
{
	static char path[1024] = { 0 };
	const char  *tempPath  = [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject] UTF8String];
	Q_strncpyz(path, tempPath, sizeof(path));
	return (const char *)path;
}

/**
 * @brief Checks for OSX' App Translocation
 */
bool IsTranslocatedURL(CFURLRef currentURL, CFURLRef *originalURL)
{
	if (currentURL == NULL)
	{
		return false;
	}

	if (floor(NSAppKitVersionNumber) <= 1404)
	{
		return false;
	}

	void *handle = dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_LAZY);
	if (handle == NULL)
	{
		return false;
	}

	bool isTranslocated = false;

	Boolean (*mySecTranslocateIsTranslocatedURL)(CFURLRef path, bool *isTranslocated, CFErrorRef *__nullable error);
	mySecTranslocateIsTranslocatedURL = dlsym(handle, "SecTranslocateIsTranslocatedURL");
	if (mySecTranslocateIsTranslocatedURL != NULL)
	{
		if (mySecTranslocateIsTranslocatedURL(currentURL, &isTranslocated, NULL))
		{
			if (isTranslocated)
			{
				if (originalURL != NULL)
				{
					CFURLRef __nullable (*mySecTranslocateCreateOriginalPathForURL)(CFURLRef translocatedPath, CFErrorRef *__nullable error);
					mySecTranslocateCreateOriginalPathForURL = dlsym(handle, "SecTranslocateCreateOriginalPathForURL");
					if (mySecTranslocateCreateOriginalPathForURL != NULL)
					{
						*originalURL = mySecTranslocateCreateOriginalPathForURL((CFURLRef)currentURL, NULL);
					}
					else
					{
						*originalURL = NULL;
					}
				}
			}
		}
	}

	dlclose(handle);

	return isTranslocated;
}

/**
 * @brief Check for OSX Quarantine, remove the attributes und restart the app
 * @return int: 0 = no action required, 1 = relaunch after dequarantine, >=2 = error, show modal
 */
int needsOSXQuarantineFix()
{
	bool  isQuarantined;
	bool  dialogReturn;
	int   taskRetVal;

	// appPath contains complete path including etl.app
	NSURL *appPath = [NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]];
	NSURL *newPath = nil;

	//does the app run in a translocated path?
	isQuarantined = IsTranslocatedURL((CFURLRef) appPath, &newPath);

	if (isQuarantined)
	{
		// strip etl.app from string to get the directory
		NSMutableString *tempPath = [NSMutableString stringWithString: [newPath.path stringByReplacingOccurrencesOfString:@"/etl.app" withString:@""]];
		NSString *tempPathBin = @"/etl.app";

		// assemble dialog text
		NSMutableString *permissiontext = [NSMutableString stringWithString: @"The game runs in a hidden folder, to prevent possible dangerous apps. As this prevents loading the game files, a command needs to be executed to run the game from its original path.\r\n\r\nShould the following command be executed now?\r\n\r\n/usr/bin/xattr -cr "];
		[permissiontext appendString:tempPath];

		// ask user if we should fix it programmatically
		dialogReturn = Sys_Dialog(DT_YES_NO, [permissiontext UTF8String], "App Translocation detected");
		if (dialogReturn == DR_YES)
		{
			// remove quarantine flag
			@try
			{
				NSTask *xattrTask = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/xattr" arguments:@[@"-cr", (NSURL *)tempPath]];
				[xattrTask waitUntilExit];
				taskRetVal = xattrTask.terminationStatus;
				if (taskRetVal != 0)
				{
					char retstr[410];
					snprintf(retstr, 400, "xattr error return value: %d", taskRetVal);
					Sys_Dialog(DT_ERROR, retstr, "Can't remove app quarantine");
					return 5;
				}
			}
			@catch (NSException *exception)
			{
				Sys_Dialog(DT_ERROR, [exception.reason UTF8String], "Can't remove app quarantine");
				return 2;
			}

			// relaunch, using 'open'
			@try
			{
				// readd "/etl.app" to path
				[tempPath appendString:tempPathBin];

				[NSTask launchedTaskWithLaunchPath:@"/usr/bin/open" arguments:@[@"-n", @"-a", tempPath]];
			}
			@catch (NSException *exception)
			{
				return 3;
			}

			//shutdown this instance
			return 1;
		}
		else
		{
			NSMutableString *errortext = [NSMutableString stringWithString: @"Running ET Legacy with enabled App Translocation isn't possible. Please remove the quarantine flag by using the following command in the terminal and restart the game:\r\n\r\nxattr -cr "];
			[errortext appendString:tempPath];
			[errortext appendString:@"\r\n\r\nFor more information please go to:\r\n\r\nhttps://dev.etlegacy.com/projects/etlegacy/wiki/Mac_OS_X"];
			Sys_Dialog(DT_ERROR, [errortext UTF8String], "App Translocation detected");
			return 4;
		}
	}
	else
	{
		return 0;
	}
}
