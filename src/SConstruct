# -*- mode: python -*-
# ET build script
# TTimo <ttimo@idsoftware.com>
# http://scons.sourceforge.net

import sys, os, time, commands, re, pickle, StringIO, popen2, commands, pdb, zipfile, string, tempfile
import SCons

import scons_utils

conf_filename='site.conf'
# choose configuration variables which should be saved between runs
# ( we handle all those as strings )
serialized=[ 'CC', 'CXX', 'JOBS', 'BUILD', 'BUILD_ROOT', 'DEDICATED', 'TARGET_CORE', 'TARGET_BSPC', 'TARGET_CGAME', 'TARGET_GAME', 'TARGET_UI', 'MASTER' ]

# help -------------------------------------------

Help("""
Usage: scons [OPTIONS] [TARGET] [CONFIG]

[OPTIONS] and [TARGET] are covered in command line options, use scons -H

[CONFIG]: KEY="VALUE" [...]
a number of configuration options saved between runs in the """ + conf_filename + """ file
erase """ + conf_filename + """ to start with default settings again

CC (default gcc)
CXX (default g++)
	Specify C and C++ compilers (defaults gcc and g++)
	ex: CC="gcc-3.3"
	You can use ccache and distcc, for instance:
	CC="ccache distcc gcc" CXX="ccache distcc g++"

JOBS (default 1)
	Parallel build

BUILD (default debug)
	Use debug-all/debug/release to select build settings
	ex: BUILD="release"
	debug-all: no optimisations, debugging symbols
	debug: -O -g
	release: all optimisations, including CPU target etc.
	
DEDICATED (default 2)
	Control regular / dedicated type of build:
	0 - client
	1 - dedicated server
	2 - both

TARGET_CORE (default 1)
	Build engine

TARGET_BSPC (default 0)
	Build bspc utility

TARGET_GAME (default 1)
	Build game module

TARGET_CGAME (default 1)
	Build cgame module

TARGET_UI (default 1)
	Build ui module

BUILD_ROOT (default 'build')
	change the build root directory
	
NOCONF (default 0, not saved)
	ignore site configuration and use defaults + command line only

MASTER (default '')
	set to override the default master server address

COPYBINS (default 0, not saved)
	copy the binaries in a ready-to-release format

BUILDMPBIN (default 0, not saved)
	build mp_bin.pk3 using bin/ directories and game binaries for all platforms
"""
)

# end help ---------------------------------------

# sanity -----------------------------------------

EnsureSConsVersion( 0, 96 )

# end sanity -------------------------------------

# system detection -------------------------------

# CPU type
cpu = commands.getoutput('uname -m')
dll_cpu = '???' # grmbl, alternative naming for .so
exp = re.compile('.*i?86.*')
if exp.match(cpu):
	cpu = 'x86'
	dll_cpu = 'i386'
else:
	cpu = commands.getoutput('uname -p')
	if ( cpu == 'powerpc' ):
		cpu = 'ppc'
		dll_cpu = cpu
	else:
		cpu = 'cpu'
		dll_cpu = cpu
OS = commands.getoutput( 'uname -s' )
if ( OS == 'Darwin' ):
	print 'EXPERIMENTAL - INCOMPLETE'
	print 'Use the XCode projects to compile ET universal binaries for OSX'
	cpu = 'osx'
	dll_cpu = 'osx'

print 'cpu: ' + cpu

# end system detection ---------------------------

# default settings -------------------------------

CC = 'gcc -m32'
CXX = 'g++ -m32'
JOBS = '1'
BUILD = 'debug'
DEDICATED = '2'
TARGET_CORE = '1'
TARGET_GAME = '1'
TARGET_CGAME = '1'
TARGET_UI = '1'
TARGET_BSPC = '0'
BUILD_ROOT = 'build'
NOCONF = '0'
MASTER = ''
COPYBINS = '0'
BUILDMPBIN = '0'

# end default settings ---------------------------

# arch detection ---------------------------------

def gcc_major():
	major = os.popen( CC + ' -dumpversion' ).read().strip()
	major = re.sub('^([^.]+)\\..*$', '\\1', major)
	print 'gcc major: %s' % major
	return major

gcc3 = (gcc_major() != '2')

def gcc_is_mingw():
	mingw = os.popen( CC + ' -dumpmachine' ).read()
	return re.search('mingw', mingw) != None

if gcc_is_mingw():
	g_os = 'win32'
elif OS == 'Darwin':
	g_os = 'Darwin'
else:
	g_os = 'Linux'
print 'OS: %s' % g_os

# end arch detection -----------------------------

# site settings ----------------------------------

if ( not ARGUMENTS.has_key( 'NOCONF' ) or ARGUMENTS['NOCONF'] != '1' ):
	site_dict = {}
	if (os.path.exists(conf_filename)):
		site_file = open(conf_filename, 'r')
		p = pickle.Unpickler(site_file)
		site_dict = p.load()
		print 'Loading build configuration from ' + conf_filename + ':'
		for k, v in site_dict.items():
			exec_cmd = k + '=\'' + v + '\''
			print '  ' + exec_cmd
			exec(exec_cmd)
else:
	print 'Site settings ignored'

# end site settings ------------------------------

# command line settings --------------------------

for k in ARGUMENTS.keys():
	exec_cmd = k + '=\'' + ARGUMENTS[k] + '\''
	print 'Command line: ' + exec_cmd
	exec( exec_cmd )

# end command line settings ----------------------

# save site configuration ----------------------

if ( not ARGUMENTS.has_key( 'NOCONF' ) or ARGUMENTS['NOCONF'] != '1' ):
	for k in serialized:
		exec_cmd = 'site_dict[\'' + k + '\'] = ' + k
		exec(exec_cmd)

	site_file = open(conf_filename, 'w')
	p = pickle.Pickler(site_file)
	p.dump(site_dict)
	site_file.close()

# end save site configuration ------------------

# general configuration, target selection --------

g_build = BUILD_ROOT + '/' + BUILD

SConsignFile( 'scons.signatures' )

SetOption('num_jobs', JOBS)

if ( OS == 'Linux' ):
	LINK = CC
else:
	LINK = CXX

# common flags
# BASE + CORE + OPT for engine
# BASE + GAME + OPT for game

BASECPPFLAGS = [ ]
CORECPPPATH = [ ]
CORELIBPATH = [ ]
CORECPPFLAGS = [ ]
GAMECPPFLAGS = [ ]
BASELINKFLAGS = [ ]
CORELINKFLAGS = [ ]

# for release build, further optimisations that may not work on all files
OPTCPPFLAGS = [ ]

if ( OS == 'Darwin' ):
	BASECPPFLAGS += [ '-D__MACOS__', '-Wno-long-double', '-Wno-unknown-pragmas', '-Wno-trigraphs', '-fpascal-strings', '-fasm-blocks', '-Wreturn-type', '-Wunused-variable', '-ffast-math', '-fno-unsafe-math-optimizations', '-fvisibility=hidden', '-mmacosx-version-min=10.4', '-isysroot', '/Developer/SDKs/MacOSX10.4u.sdk' ]
	BASECPPFLAGS += [ '-arch', 'i386' ]
	BASELINKFLAGS += [ '-arch', 'i386' ]

BASECPPFLAGS.append( '-pipe' )
# warn all
BASECPPFLAGS.append( '-Wall' )
# don't wrap gcc messages
BASECPPFLAGS.append( '-fmessage-length=0' )

if ( BUILD == 'debug-all' ):
	BASECPPFLAGS.append( '-g' )
	BASECPPFLAGS.append( '-D_DEBUG' )
elif ( BUILD == 'debug' ):
	BASECPPFLAGS.append( '-g' )
	BASECPPFLAGS.append( '-O1' )
	BASECPPFLAGS.append( '-D_DEBUG' )
elif ( BUILD == 'release' ):
	BASECPPFLAGS.append( '-DNDEBUG' )
	if ( OS == 'Linux' ):
		# -fomit-frame-pointer: gcc manual indicates -O sets this implicitely,
		# only if that doesn't affect debugging
		# on Linux, this affects backtrace capability, so I'm assuming this is needed
		# -finline-functions: implicit at -O3
		# -fschedule-insns2: implicit at -O3
		# -funroll-loops ?
		# -mfpmath=sse -msse ?
		OPTCPPFLAGS = [ '-O3', '-march=i686', '-Winline', '-ffast-math', '-fomit-frame-pointer', '-finline-functions', '-fschedule-insns2' ]
	elif ( OS == 'Darwin' ):
		OPTCPPFLAGS = []
else:
	print 'Unknown build configuration ' + BUILD
	sys.exit(0)

# create the build environements
g_base_env = Environment( ENV = os.environ, CC = CC, CXX = CXX, LINK = LINK, CPPFLAGS = BASECPPFLAGS, LINKFLAGS = BASELINKFLAGS, CPPPATH = CORECPPPATH, LIBPATH = CORELIBPATH )
scons_utils.SetupUtils( g_base_env )

g_env = g_base_env.Clone()

g_env['CPPFLAGS'] += OPTCPPFLAGS
g_env['CPPFLAGS'] += CORECPPFLAGS
g_env['LINKFLAGS'] += CORELINKFLAGS

if ( OS == 'Darwin' ):
	# configure for dynamic bundle
	g_env['SHLINKFLAGS'] = '$LINKFLAGS -bundle -flat_namespace -undefined suppress'
	g_env['SHLIBSUFFIX'] = '.so'

# maintain this dangerous optimization off at all times
g_env.Append( CPPFLAGS = '-fno-strict-aliasing' )

if ( int(JOBS) > 1 ):
	print 'Using buffered process output'
	scons_utils.SetupBufferedOutput( g_env )

# mark the globals

local_dedicated = 0
# curl
local_curl = 0	# config selection
curl_lib = []

GLOBALS = 'g_env OS g_os BUILD local_dedicated curl_lib local_curl MASTER gcc3 cpu'

# end general configuration ----------------------

# win32 cross compilation ----------------------

if g_os == 'win32' and os.name != 'nt':
	# mingw doesn't define the cpu type, but our only target is x86
	g_env.Append(CPPDEFINES = '_M_IX86=400')
	g_env.Append(LINKFLAGS = '-static-libgcc')
	# scons doesn't support cross-compiling, so set these up manually
	g_env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME'] = 1
	g_env['WIN32DEFSUFFIX']	= '.def'
	g_env['PROGSUFFIX']	= '.exe'
	g_env['SHLIBSUFFIX']	= '.dll'
	g_env['SHCCFLAGS']	= '$CCFLAGS'

# end win32 cross compilation ------------------

# targets ----------------------------------------

toplevel_targets = []

# build curl if needed
if ( TARGET_CORE == '1' and DEDICATED != '1' and OS != 'Darwin' ):
	# 1: debug, 2: release
	if ( BUILD == 'release' ):
		local_curl = 2
	else:
		local_curl = 1
	Export( 'GLOBALS ' + GLOBALS )
	curl_lib = SConscript( 'SConscript.curl' )

if ( TARGET_CORE == '1' ):
	if ( DEDICATED == '0' or DEDICATED == '2' ):
		local_dedicated = 0
		Export( 'GLOBALS ' + GLOBALS )
		BuildDir( g_build + '/core', '.', duplicate = 0 )
		et = SConscript( g_build + '/core/SConscript.core' )
		if ( g_os == 'win32' ):
			toplevel_targets.append( InstallAs( '#et.exe', et ) )
		else:
			toplevel_targets.append( InstallAs( '#et.' + cpu, et ) )

	if ( DEDICATED == '1' or DEDICATED == '2' ):
		local_dedicated = 1
		Export( 'GLOBALS ' + GLOBALS )
		BuildDir( g_build + '/dedicated', '.', duplicate = 0 )
		etded = SConscript( g_build + '/dedicated/SConscript.core' )
		if ( g_os == 'win32' ):
			toplevel_targets.append( InstallAs( '#etded.exe', etded ) )
		else:
			toplevel_targets.append( InstallAs( '#etded.' + cpu, etded ) )

if ( TARGET_BSPC == '1' ):
	Export( 'GLOBALS ' + GLOBALS )
	BuildDir( g_build + '/bspc', '.', duplicate = 0 )
	bspc = SConscript( g_build + '/bspc/SConscript.bspc' )
	toplevel_targets.append( InstallAs( '#bspc.' + cpu, bspc ) )

if ( TARGET_GAME == '1' ):
	Export( 'GLOBALS ' + GLOBALS )
	BuildDir( g_build + '/game', '.', duplicate = 0 )
	game = SConscript( g_build + '/game/SConscript.game' )
	toplevel_targets.append( InstallAs( '#qagame.mp.%s.so' % dll_cpu, game ) )

if ( TARGET_CGAME == '1' ):
	Export( 'GLOBALS ' + GLOBALS )
	BuildDir( g_build + '/cgame', '.', duplicate = 0 )
	cgame = SConscript( g_build + '/cgame/SConscript.cgame' )
	toplevel_targets.append( InstallAs( '#cgame.mp.%s.so' % dll_cpu, cgame ) )

if ( TARGET_UI == '1' ):
	Export( 'GLOBALS ' + GLOBALS )
	BuildDir( g_build + '/ui', '.', duplicate = 0 )
	ui = SConscript( g_build + '/ui/SConscript.ui' )
	toplevel_targets.append( InstallAs( '#ui.mp.%s.so' % dll_cpu, ui ) )

class CopyBins(scons_utils.idSetupBase):
	def copy_bins( self, target, source, env ):
		for i in source:
			j = os.path.normpath( os.path.join( os.path.dirname( i.abspath ), '../bin', os.path.basename( i.abspath ) ) )
			self.SimpleCommand( 'cp ' + i.abspath + ' ' + j )
			if ( OS == 'Linux' ):
				self.SimpleCommand( 'strip ' + j )
			else:
				# see strip and otool man pages on mac
				self.SimpleCommand( 'strip -ur ' + j )

copybins_target = []
if ( COPYBINS != '0' ):
	copy = CopyBins()
	copybins_target.append( Command( 'copybins', toplevel_targets, Action( copy.copy_bins ) ) )

class MpBin(scons_utils.idSetupBase):
	def mp_bin( self, target, source, env ):
		temp_dir = tempfile.mkdtemp( prefix = 'mp_bin' )
		self.SimpleCommand( 'cp ../bin/ui* ' + temp_dir )
		self.SimpleCommand( 'cp ../bin/cgame* ' + temp_dir )
		# zip the mac bundles
		mac_bundle_dir = tempfile.mkdtemp( prefix = 'mp_mac' )
		self.SimpleCommand( 'cp -R "../bin/Wolfenstein ET.app/Contents/Resources/ui_mac.bundle" ' + mac_bundle_dir )
		self.SimpleCommand( 'cp -R "../bin/Wolfenstein ET.app/Contents/Resources/cgame_mac.bundle" ' + mac_bundle_dir )
		self.SimpleCommand( 'find %s -name \.svn | xargs rm -rf' % mac_bundle_dir )
		self.SimpleCommand( 'cd %s ; zip -r -D %s/ui_mac.zip ui_mac.bundle ; mv %s/ui_mac.zip %s/ui_mac' % ( mac_bundle_dir, temp_dir, temp_dir, temp_dir ) )
		self.SimpleCommand( 'cd %s ; zip -r -D %s/cgame_mac.zip cgame_mac.bundle ; mv %s/cgame_mac.zip %s/cgame_mac' % ( mac_bundle_dir, temp_dir, temp_dir, temp_dir ) )
		mp_bin_path = os.path.abspath( os.path.join ( os.getcwd(), '../etmain/mp_bin.pk3' ) )
		self.SimpleCommand( 'cd %s ; zip -r -D %s *' % ( temp_dir, mp_bin_path ) )

if ( BUILDMPBIN != '0' ):
	mp_bin = MpBin()
	mpbin_target = Command( 'mp_bin', toplevel_targets + copybins_target, Action( mp_bin.mp_bin ) )

# end targets ------------------------------------
