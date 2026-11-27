/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
#import <Cocoa/Cocoa.h>

#ifndef DEDICATED
#include "../sdl/sdl_defs.h"
#include <CommonCrypto/CommonDigest.h>
#endif


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
		[alert setAlertStyle: NSAlertStyleCritical];
	}
	else
	{
		[alert setAlertStyle: NSAlertStyleWarning];
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
 * @brief Check for OSX Quarantine, remove the attributes and restart the app
 * @return int: 0 = no action required, 1 = relaunch after dequarantine, >=2 = error, show modal
 */
int OSX_NeedsQuarantineFix()
{
	bool  isQuarantined;
	bool  dialogReturn;
	int   taskRetVal;

	// appPath contains complete path including "ET Legacy.app"
	NSURL *appPath = [NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]];
	NSURL *realPath = nil;

	//does the app run in a translocated path?
	isQuarantined = IsTranslocatedURL((CFURLRef) appPath, (CFURLRef *) &realPath);

	if (isQuarantined)
	{
		if(!realPath)
		{
			Sys_Dialog(DT_ERROR, "Could not detect real application path", "Can't remove app quarantine");
			return 2;
		}

		NSString *installPath = [[realPath path] stringByDeletingLastPathComponent];

		// assemble dialog text
		NSMutableString *permissiontext = [NSMutableString stringWithString: @"The game runs in a hidden folder, to prevent possible dangerous apps. As this prevents loading the game files, a command needs to be executed to run the game from its original path.\r\n\r\nShould the following command be executed now?\r\n\r\n/usr/bin/xattr -cr "];
		[permissiontext appendString:installPath];

		// ask user if we should fix it programmatically
		dialogReturn = Sys_Dialog(DT_YES_NO, [permissiontext UTF8String], "App Translocation detected");
		if (dialogReturn == DR_YES)
		{
			// remove quarantine flag
			@try
			{
				NSTask *xattrTask = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/xattr" arguments:@[@"-cr", installPath]];
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
				[NSTask launchedTaskWithLaunchPath:@"/usr/bin/open" arguments:@[@"-n", @"-a", realPath.path]];
			}
			@catch (NSException *exception)
			{
				Sys_Dialog(DT_ERROR, [exception.reason UTF8String], "Failed to relaunch the game");
				return 3;
			}

			//shutdown this instance
			return 1;
		}
		else
		{
			NSMutableString *errortext = [NSMutableString stringWithString: @"Running ET: Legacy with enabled App Translocation isn't possible. Please remove the quarantine flag by using the following command in the terminal and restart the game:\r\n\r\nxattr -cr "];
			[errortext appendString:installPath];
			[errortext appendString:@"\r\n\r\nFor more information please go to:\r\n\r\nhttps://github.com/etlegacy/etlegacy/wiki/Mac-OS-X"];
			Sys_Dialog(DT_ERROR, [errortext UTF8String], "App Translocation detected");
			return 4;
		}
	}
	else
	{
		return 0;
	}
}

#ifndef DEDICATED

/*
 * Game data setup: the macOS package has no installer that sets up the
 * original game data like the Windows and Linux ones, so the client offers it
 * on its first start. It downloads the original, free Wolfenstein: Enemy
 * Territory 2.60d release for macOS (its license agreement is shown and
 * accepted first, the image itself asks for the same one), checks it and
 * copies the data files from it into fs_homepath's etmain.
 */

static const char *gameDataUrls[] =
{
	"https://cdn.splashdamage.com/downloads/games/wet/WolfET.2.60d.dmg",
	"https://mirror.etlegacy.com/vanilla-files/WolfET.2.60d.dmg",
};

#define GAMEDATA_IMAGE_NAME   "WolfET.2.60d.dmg"
#define GAMEDATA_IMAGE_SIZE   269563137.0
#define GAMEDATA_IMAGE_SHA256 "764cebe732ad3bd79e278371c489fe129e15b6191100396422fa82a3667798a3"
#define GAMEDATA_IMAGE_ETMAIN "Wolfenstein ET/etmain"

typedef struct
{
	const char *name;       ///< path inside etmain
	const char *sha256;
	qboolean required;
} gameDataFile_t;

static const gameDataFile_t gameDataFiles[] =
{
	{ "pak0.pk3",          "712966b20e06523fe81419516500e499c86b2b4fec823856ddbd333fcb3d26e5", qtrue  },
	{ "pak1.pk3",          "5610fd749024405b4425a7ce6397e58187b941d22092ef11d4844b427df53e5d", qtrue  },
	{ "pak2.pk3",          "a48ab749a1a12ab4d9137286b1f23d642c29da59845b2bafc8f64e052cf06f3e", qtrue  },
	{ "video/etintro.roq", "9710b6c7b0654109eecd0273ccee7868e49103cc796ecacaafe7f934573a8c7e", qfalse },
};

/**
 * @brief SHA-256 of a file
 * @return Lowercase hex digest (autoreleased), or nil if the file can't be read
 */
static NSString *OSX_FileSha256(NSString *path)
{
	NSFileHandle    *file = [NSFileHandle fileHandleForReadingAtPath: path];
	CC_SHA256_CTX   ctx;
	unsigned char   digest[CC_SHA256_DIGEST_LENGTH];
	NSMutableString *hex;
	int             i;

	if (!file)
	{
		return nil;
	}

	CC_SHA256_Init(&ctx);
	for (;;)
	{
		@autoreleasepool
		{
			NSData *data = [file readDataOfLength: 4 * 1024 * 1024];

			if (![data length])
			{
				break;
			}
			CC_SHA256_Update(&ctx, [data bytes], (CC_LONG)[data length]);
		}
	}
	[file closeFile];
	CC_SHA256_Final(digest, &ctx);

	hex = [NSMutableString stringWithCapacity: CC_SHA256_DIGEST_LENGTH * 2];
	for (i = 0; i < CC_SHA256_DIGEST_LENGTH; i++)
	{
		[hex appendFormat: @"%02x", digest[i]];
	}
	return hex;
}

/**
 * @brief Copies the game data files from an etmain folder into fs_homepath's etmain
 * @return nil on success, otherwise the reason (autoreleased)
 */
static NSString *OSX_CopyGameData(NSString *srcEtmain, NSString *dstEtmain)
{
	NSFileManager *fm    = [NSFileManager defaultManager];
	NSError       *error = nil;
	size_t        i;

	for (i = 0; i < ARRAY_LEN(gameDataFiles); i++)
	{
		NSString *name = [NSString stringWithUTF8String: gameDataFiles[i].name];
		NSString *src  = [srcEtmain stringByAppendingPathComponent: name];
		NSString *dst  = [dstEtmain stringByAppendingPathComponent: name];
		NSString *part = [dst stringByAppendingString: @".part"];
		NSString *hash;

		if (![fm fileExistsAtPath: src])
		{
			if (gameDataFiles[i].required)
			{
				return [NSString stringWithFormat: @"%@ was not found in %@.", name, srcEtmain];
			}
			continue;
		}

		// a file that is already in place stays
		if ([fm fileExistsAtPath: dst] && [OSX_FileSha256(dst) isEqualToString: [NSString stringWithUTF8String: gameDataFiles[i].sha256]])
		{
			continue;
		}

		hash = OSX_FileSha256(src);
		if (![hash isEqualToString: [NSString stringWithUTF8String: gameDataFiles[i].sha256]])
		{
			if (gameDataFiles[i].required)
			{
				return [NSString stringWithFormat: @"%@ is not the file of the original release (wrong checksum).", src];
			}
			continue;
		}

		if (![fm createDirectoryAtPath: [dst stringByDeletingLastPathComponent] withIntermediateDirectories: YES attributes: nil error: &error])
		{
			return [NSString stringWithFormat: @"Can't create %@: %@", [dst stringByDeletingLastPathComponent], [error localizedDescription]];
		}

		// copy under a temporary name, so an interrupted copy never leaves a broken file behind
		[fm removeItemAtPath: part error: nil];
		if (![fm copyItemAtPath: src toPath: part error: &error])
		{
			[fm removeItemAtPath: part error: nil];
			return [NSString stringWithFormat: @"Can't copy %@: %@", name, [error localizedDescription]];
		}
		if (rename([part fileSystemRepresentation], [dst fileSystemRepresentation]) != 0)
		{
			[fm removeItemAtPath: part error: nil];
			return [NSString stringWithFormat: @"Can't write %@: %s", dst, strerror(errno)];
		}
	}

	return nil;
}

/**
 * @brief Runs hdiutil with the given arguments
 * @param[in] answer Written to its input (the license prompt of the image), or nil
 * @return nil on success, otherwise the reason (autoreleased)
 */
static NSString *OSX_RunHdiutil(NSArray *arguments, NSString *answer)
{
	NSTask       *task   = [[[NSTask alloc] init] autorelease];
	NSPipe       *input  = [NSPipe pipe];
	NSPipe       *errors = [NSPipe pipe];
	NSDictionary *env;
	NSData       *errorText;

	// show the license as plain text instead of in a pager
	env = [NSDictionary dictionaryWithObjectsAndKeys: @"/bin/cat", @"PAGER", @"/usr/bin:/bin:/usr/sbin:/sbin", @"PATH", nil];

	[task setLaunchPath: @"/usr/bin/hdiutil"];
	[task setArguments: arguments];
	[task setEnvironment: env];
	[task setStandardInput: input];
	[task setStandardOutput: [NSFileHandle fileHandleWithNullDevice]];
	[task setStandardError: errors];

	@try
	{
		[task launch];
	}
	@catch (NSException *exception)
	{
		return [NSString stringWithFormat: @"Can't run hdiutil: %@", [exception reason]];
	}

	if (answer)
	{
		[[input fileHandleForWriting] writeData: [answer dataUsingEncoding: NSUTF8StringEncoding]];
	}
	[[input fileHandleForWriting] closeFile];

	errorText = [[errors fileHandleForReading] readDataToEndOfFile];
	[task waitUntilExit];

	if ([task terminationStatus] != 0)
	{
		NSString *text = [[[NSString alloc] initWithData: errorText encoding: NSUTF8StringEncoding] autorelease];

		return [NSString stringWithFormat: @"hdiutil %@ failed: %@", [arguments objectAtIndex: 0],
		        [text stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceAndNewlineCharacterSet]]];
	}
	return nil;
}

/**
 * @brief Checks the release image, mounts it and copies the game data from it
 * @return nil on success, otherwise the reason (autoreleased)
 */
static NSString *OSX_InstallFromImage(NSString *image, NSString *workDir, NSString *dstEtmain)
{
	NSString *mountPoint = [workDir stringByAppendingPathComponent: @"mnt"];
	NSString *result;

	if (![OSX_FileSha256(image) isEqualToString: @GAMEDATA_IMAGE_SHA256])
	{
		return [NSString stringWithFormat: @"%@ is not the original Wolfenstein: Enemy Territory 2.60d image (wrong checksum).", image];
	}

	[[NSFileManager defaultManager] createDirectoryAtPath: mountPoint withIntermediateDirectories: YES attributes: nil error: nil];

	// the image asks for the license agreement the user accepted in our dialog
	result = OSX_RunHdiutil([NSArray arrayWithObjects: @"attach", image, @"-readonly", @"-nobrowse", @"-noautoopen", @"-noverify",
	                         @"-mountpoint", mountPoint, nil], @"Y\n");
	if (result)
	{
		return result;
	}

	result = OSX_CopyGameData([mountPoint stringByAppendingPathComponent: @GAMEDATA_IMAGE_ETMAIN], dstEtmain);

	OSX_RunHdiutil([NSArray arrayWithObjects: @"detach", mountPoint, @"-force", @"-quiet", nil], nil);

	return result;
}

typedef enum
{
	GAMEDATA_RUNNING,
	GAMEDATA_DONE,
	GAMEDATA_FAILED,
	GAMEDATA_CANCELLED
} gameDataState_t;

/**
 * @brief Downloads and installs the game data in a modal progress window
 */
@interface ETLGameDataSetup : NSObject <NSURLSessionDownloadDelegate>
{
	NSWindow            *window;
	NSTextField         *label;
	NSProgressIndicator *bar;
	NSButton            *cancelButton;
	NSURLSession        *session;
	NSString            *workDir;
	NSString            *dstEtmain;
	NSString            *imagePath;
	NSString            *error;
	unsigned int        urlIndex;
	BOOL                downloaded;
	BOOL                cancelled;
	gameDataState_t     state;
}
- (id)initWithEtmain:(NSString *)etmain workDir:(NSString *)dir;
- (gameDataState_t)runWithSource:(NSString *)source;
- (NSString *)error;
@end

@implementation ETLGameDataSetup

- (id)initWithEtmain:(NSString *)etmain workDir:(NSString *)dir
{
	NSView *content;

	self = [super init];
	if (!self)
	{
		return nil;
	}

	dstEtmain = [etmain copy];
	workDir   = [dir copy];

	window = [[NSPanel alloc] initWithContentRect: NSMakeRect(0, 0, 480, 120)
	          styleMask: NSWindowStyleMaskTitled
	          backing: NSBackingStoreBuffered
	          defer: NO];
	[window setTitle: @"ET: Legacy"];
	[window setReleasedWhenClosed: NO];
	content = [window contentView];

	label = [[NSTextField alloc] initWithFrame: NSMakeRect(20, 78, 440, 22)];
	[label setEditable: NO];
	[label setBordered: NO];
	[label setDrawsBackground: NO];
	[content addSubview: label];

	bar = [[NSProgressIndicator alloc] initWithFrame: NSMakeRect(20, 50, 440, 20)];
	[bar setStyle: NSProgressIndicatorStyleBar];
	[bar setMinValue: 0.0];
	[bar setMaxValue: 1.0];
	[content addSubview: bar];

	cancelButton = [[NSButton alloc] initWithFrame: NSMakeRect(370, 12, 96, 32)];
	[cancelButton setTitle: @"Cancel"];
	[cancelButton setBezelStyle: NSBezelStyleRounded];
	[cancelButton setTarget: self];
	[cancelButton setAction: @selector(cancel:)];
	[content addSubview: cancelButton];

	return self;
}

- (void)dealloc
{
	[window release];
	[label release];
	[bar release];
	[cancelButton release];
	[workDir release];
	[dstEtmain release];
	[imagePath release];
	[error release];
	[super dealloc];
}

- (NSString *)error
{
	return error;
}

// all of the methods below that touch the window run on the main thread

- (void)setStatus:(NSString *)text progress:(double)progress
{
	[label setStringValue: text];
	if (progress < 0)
	{
		[bar setIndeterminate: YES];
		[bar startAnimation: nil];
	}
	else
	{
		[bar setIndeterminate: NO];
		[bar setDoubleValue: progress];
	}
}

- (void)finish:(gameDataState_t)result reason:(NSString *)reason
{
	if (state != GAMEDATA_RUNNING)
	{
		return;
	}
	state = result;
	[error release];
	error = [reason copy];

	[NSApp stopModal];
	// stopModal takes effect after the next event
	[NSApp postEvent: [NSEvent otherEventWithType: NSEventTypeApplicationDefined location: NSZeroPoint modifierFlags: 0
	                   timestamp: 0 windowNumber: 0 context: nil subtype: 0 data1: 0 data2: 0]
	 atStart: YES];
}

- (void)cancel:(id)sender
{
	cancelled = YES;
	[cancelButton setEnabled: NO];
	[self setStatus: @"Cancelling..." progress: -1];
	[session invalidateAndCancel];
}

// installs from the image or folder on a background queue
- (void)installFrom:(NSString *)source isImage:(BOOL)isImage
{
	dispatch_async(dispatch_get_main_queue(), ^{
		[cancelButton setEnabled: NO];
		[self setStatus: @"Checking and copying the game data files..." progress: -1];
	});

	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		@autoreleasepool
		{
			NSString *reason;

			if (isImage)
			{
				reason = OSX_InstallFromImage(source, workDir, dstEtmain);
			}
			else
			{
				reason = OSX_CopyGameData(source, dstEtmain);
			}

			[reason retain];
			dispatch_async(dispatch_get_main_queue(), ^{
				[self finish: reason ? GAMEDATA_FAILED : GAMEDATA_DONE reason: reason];
				[reason release];
			});
		}
	});
}

- (void)startDownload
{
	NSURL *url = [NSURL URLWithString: [NSString stringWithUTF8String: gameDataUrls[urlIndex]]];

	downloaded = NO;
	[self setStatus: [NSString stringWithFormat: @"Downloading Wolfenstein: Enemy Territory 2.60d from %@...", [url host]] progress: 0];
	[[session downloadTaskWithURL: url] resume];
}

/**
 * @param[in] source nil to download the release image, otherwise a chosen image or folder
 */
- (gameDataState_t)runWithSource:(NSString *)source
{
	BOOL isDir = NO;

	state     = GAMEDATA_RUNNING;
	cancelled = NO;
	urlIndex  = 0;
	[cancelButton setEnabled: YES];
	[[NSFileManager defaultManager] createDirectoryAtPath: workDir withIntermediateDirectories: YES attributes: nil error: nil];

	if (!source)
	{
		// the delegate queue is a serial background queue
		session = [NSURLSession sessionWithConfiguration: [NSURLSessionConfiguration defaultSessionConfiguration] delegate: self delegateQueue: nil];
		[session retain];
		[self startDownload];
	}
	else if ([[NSFileManager defaultManager] fileExistsAtPath: source isDirectory: &isDir] && isDir)
	{
		NSString *etmain = source;

		// accept the installation folder or its etmain
		if ([[NSFileManager defaultManager] fileExistsAtPath: [source stringByAppendingPathComponent: @"etmain/pak0.pk3"]])
		{
			etmain = [source stringByAppendingPathComponent: @"etmain"];
		}
		[self installFrom: etmain isImage: NO];
	}
	else
	{
		[self installFrom: source isImage: YES];
	}

	[window center];
	[NSApp runModalForWindow: window];
	[window orderOut: nil];

	[session finishTasksAndInvalidate];
	[session release];
	session = nil;

	return state;
}

- (void)URLSession:(NSURLSession *)s downloadTask:(NSURLSessionDownloadTask *)task didWriteData:(int64_t)written
    totalBytesWritten:(int64_t)total totalBytesExpectedToWrite:(int64_t)expected
{
	double size = expected > 0 ? (double)expected : GAMEDATA_IMAGE_SIZE;

	dispatch_async(dispatch_get_main_queue(), ^{
		if (!cancelled)
		{
			[self setStatus: [NSString stringWithFormat: @"Downloading Wolfenstein: Enemy Territory 2.60d... %.0f of %.0f MB",
			                  total / 1048576.0, size / 1048576.0] progress: total / size];
		}
	});
}

- (void)URLSession:(NSURLSession *)s downloadTask:(NSURLSessionDownloadTask *)task didFinishDownloadingToURL:(NSURL *)location
{
	NSInteger status = [(NSHTTPURLResponse *)[task response] statusCode];
	NSString  *path  = [workDir stringByAppendingPathComponent: @GAMEDATA_IMAGE_NAME];

	if (status != 200)
	{
		return;
	}

	// the downloaded file is deleted when this returns
	[[NSFileManager defaultManager] removeItemAtPath: path error: nil];
	if ([[NSFileManager defaultManager] moveItemAtPath: [location path] toPath: path error: nil])
	{
		[imagePath release];
		imagePath  = [path copy];
		downloaded = YES;
	}
}

- (void)URLSession:(NSURLSession *)s task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)taskError
{
	NSString *reason;

	if (cancelled)
	{
		dispatch_async(dispatch_get_main_queue(), ^{
			[self finish: GAMEDATA_CANCELLED reason: nil];
		});
		return;
	}

	if (downloaded)
	{
		[self installFrom: imagePath isImage: YES];
		return;
	}

	if (taskError)
	{
		reason = [NSString stringWithFormat: @"Downloading %s failed: %@", gameDataUrls[urlIndex], [taskError localizedDescription]];
	}
	else
	{
		reason = [NSString stringWithFormat: @"Downloading %s failed: HTTP status %ld", gameDataUrls[urlIndex],
		          (long)[(NSHTTPURLResponse *)[task response] statusCode]];
	}

	// try the next source
	if (++urlIndex < ARRAY_LEN(gameDataUrls))
	{
		dispatch_async(dispatch_get_main_queue(), ^{
			[self startDownload];
		});
		return;
	}

	[reason retain];
	dispatch_async(dispatch_get_main_queue(), ^{
		[self finish: GAMEDATA_FAILED reason: reason];
		[reason release];
	});
}

@end

/**
 * @brief Loads the license agreement of the original game from the app bundle
 * @return The text (autoreleased), or nil
 */
static NSString *OSX_GameLicense(void)
{
	NSString *path = [[NSBundle mainBundle] pathForResource: @"EULA_Wolfenstein_Enemy_Territory" ofType: @"txt"];

	return path ? [NSString stringWithContentsOfFile: path encoding: NSUTF8StringEncoding error: nil] : nil;
}

/**
 * @brief Asks how to get the game data
 * @return NSAlertFirstButtonReturn = download, NSAlertSecondButtonReturn = choose a file, NSAlertThirdButtonReturn = quit
 */
static NSModalResponse OSX_AskGameData(NSString *etmain, NSString *license, NSString *failure)
{
	NSAlert         *alert = [[[NSAlert alloc] init] autorelease];
	NSString        *text;
	NSModalResponse response;

	if (failure)
	{
		[alert setMessageText: @"Setting up the game data failed"];
		text = [NSString stringWithFormat: @"%@\n\nTry again, or choose the release image (%s) or the folder of an existing Wolfenstein: Enemy Territory installation.",
		        failure, GAMEDATA_IMAGE_NAME];
		[alert addButtonWithTitle: @"Try Again"];
	}
	else
	{
		NSScrollView *scroll = [[[NSScrollView alloc] initWithFrame: NSMakeRect(0, 0, 520, 260)] autorelease];
		NSTextView   *view   = [[[NSTextView alloc] initWithFrame: NSMakeRect(0, 0, 505, 260)] autorelease];

		[alert setMessageText: @"Wolfenstein: Enemy Territory game data needed"];
		text = [NSString stringWithFormat: @"ET: Legacy needs the data files of the original Wolfenstein: Enemy Territory, which are not installed. "
		                                   "It can download the free original release for macOS (%.0f MB) and copy the files into:\n\n%@\n\n"
		                                   "Downloading the release, or choosing its image or an existing installation, means accepting id Software's license agreement below.",
		        GAMEDATA_IMAGE_SIZE / 1048576.0, etmain];

		[view setString: license];
		[view setEditable: NO];
		[view setVerticallyResizable: YES];
		[[view textContainer] setWidthTracksTextView: YES];
		[scroll setDocumentView: view];
		[scroll setHasVerticalScroller: YES];
		[scroll setBorderType: NSBezelBorder];
		[alert setAccessoryView: scroll];
		[alert addButtonWithTitle: @"Accept and Download"];
	}

	[alert setInformativeText: text];
	[alert addButtonWithTitle: @"Choose..."];
	[alert addButtonWithTitle: @"Quit"];

	[NSApp activateIgnoringOtherApps: YES];
	response = [alert runModal];
	return response;
}

/**
 * @brief Lets the user choose the release image or an existing installation
 * @return The chosen path (autoreleased), or nil
 */
static NSString *OSX_ChooseGameData(void)
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];

	[panel setMessage: @"Choose " GAMEDATA_IMAGE_NAME " or the folder of a Wolfenstein: Enemy Territory installation"];
	[panel setCanChooseFiles: YES];
	[panel setCanChooseDirectories: YES];
	[panel setAllowsMultipleSelection: NO];
	[panel setAllowedFileTypes: [NSArray arrayWithObject: @"dmg"]];

	if ([panel runModal] != NSModalResponseOK)
	{
		return nil;
	}
	return [[[panel URLs] firstObject] path];
}

/**
 * @brief Offers to set up the original game data (pak0.pk3 - pak2.pk3) in etmainPath
 * @param[in] etmainPath The etmain folder of fs_homepath
 * @return qtrue if the files were installed
 */
qboolean Sys_SetupGameData(const char *etmainPath)
{
	qboolean installed = qfalse;

	// let SDL set up the application, as it does when the window opens later
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
	{
		Com_Printf("Sys_SetupGameData: can't initialize SDL video: %s\n", SDL_GetError());
		return qfalse;
	}

	@autoreleasepool
	{
		NSString         *etmain  = [NSString stringWithUTF8String: etmainPath];
		NSString         *license = OSX_GameLicense();
		NSString         *workDir = [NSTemporaryDirectory() stringByAppendingPathComponent: @"etlegacy-gamedata"];
		NSString         *failure = nil;
		ETLGameDataSetup *setup;

		if (!license)
		{
			Com_Printf("Sys_SetupGameData: the license agreement is missing from the app bundle\n");
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			return qfalse;
		}

		Com_Printf("Original game data not found, offering to set it up in %s\n", etmainPath);
		setup = [[[ETLGameDataSetup alloc] initWithEtmain: etmain workDir: workDir] autorelease];

		for (;;)
		{
			NSModalResponse choice = OSX_AskGameData(etmain, license, failure);
			NSString        *source;
			gameDataState_t result;

			if (choice == NSAlertFirstButtonReturn)
			{
				source = nil;
			}
			else if (choice == NSAlertSecondButtonReturn)
			{
				source = OSX_ChooseGameData();
				if (!source)
				{
					continue;
				}
			}
			else
			{
				Com_Printf("Game data setup declined\n");
				break;
			}

			result = [setup runWithSource: source];

			if (result == GAMEDATA_DONE)
			{
				Com_Printf("Game data installed in %s\n", etmainPath);
				installed = qtrue;
				break;
			}
			if (result == GAMEDATA_CANCELLED)
			{
				Com_Printf("Game data download cancelled\n");
				failure = nil;
				continue;
			}

			Com_Printf("Game data setup failed: %s\n", [[setup error] UTF8String]);
			// the next run replaces the setup's error
			failure = [[[setup error] retain] autorelease];
		}

		[[NSFileManager defaultManager] removeItemAtPath: workDir error: nil];
	}

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	return installed;
}

#endif // DEDICATED
