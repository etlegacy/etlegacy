#!/sw/bin/python
# there is something that sucks more than XCode: PackageMaker and the whole software installation on OSX mess
# you want to run that script as root. sudo blah etc.

import shutil, os, sys, re

import mycmds

def PrepareMain():
	try:
		shutil.rmtree( 'main/tree' )
	except:
		print 'rmtree main/tree failed'
		pass
	os.makedirs( 'main/tree' )
	
	shutil.copytree( '/Volumes/EDEN;HOME/Timothee Besset/enemy-territory', 'main/tree/Wolfenstein ET' )
	shutil.copyfile( '../../../etmain/mp_bin.pk3', 'main/tree/Wolfenstein ET/etmain/mp_bin.pk3' )
	shutil.copyfile( '../../../etmain/pak2.pk3', 'main/tree/Wolfenstein ET/etmain/pak2.pk3' )
	shutil.copytree( '../../../et-install-2.60/Docs', 'main/tree/Wolfenstein ET/Docs' )
	shutil.copyfile( '../../../et-install-2.60/etmain/description.txt', 'main/tree/Wolfenstein ET/etmain/description.txt' )
	
	# bins
	shutil.copytree( '../../../bin/Wolfenstein ET.app', 'main/tree/Wolfenstein ET/Wolfenstein ET.app' )
	shutil.copyfile( '../../../bin/rtcw_et_server', 'main/tree/Wolfenstein ET/rtcw_et_server' )
	
	# resources
	try:
		shutil.rmtree( 'main/resource' )
	except:
		print 'rmtree main/resource failed'
		pass
	os.mkdir( 'main/resource' )
	shutil.copyfile( 'main/tree/Wolfenstein ET/Docs/EULA_Wolfenstein_Enemy_Territory.txt', 'main/resource/License.txt' )

def PreparePB():
	# PB package
	try:
		shutil.rmtree( 'pb/tree' )
	except:
		print 'rmtree pb/tree failed'
		pass
	os.makedirs( 'pb/tree/Wolfenstein ET' )

	shutil.copytree( '../../pb/mac', 'pb/tree/Wolfenstein ET/pb' )
	shutil.copytree( '../../../et-install-2.60/pb/htm', 'pb/tree/Wolfenstein ET/pb/htm' )

	# resources
	try:
		shutil.rmtree( 'pb/resource' )
	except:
		print 'rmtree pb/resource failed'
		pass
	os.mkdir( 'pb/resource' )
	shutil.copyfile( '../../../et-install-2.60/pb/PB_EULA.txt', 'pb/resource/License.txt' )	

def MakeImage():
	os.system( 'rm -rf *.dmg' )
	# 99 is the "magic user" 
	mycmds.SimpleCommand( 'hdiutil create -uid 99 -gid 99 -fs HFS+ -attach -volname "Wolfenstein Enemy Territory 2.60" -megabytes 300 "tmp.dmg"' )
	
	dest = mycmds.SilentCommand( 'mount | tail -1' )
	dest = re.split( '.* on (.*) \\(.*', dest )[ 1 ]
	print 'mounted at: ' + dest

	# python copytree utils mess it up completely for some reason
	print 'Copy the content and press enter'	
	sys.stdin.readline()
	
	# convert to a compressed image to shrink it some more	
	mycmds.SimpleCommand( 'hdiutil unmount "' + dest + '"' )
	mycmds.SimpleCommand( 'hdiutil convert "tmp.dmg" -format UDCO -o "Wolfenstein Enemy Territory 2.60-2.dmg"' )

os.system( 'rm -rf *.pkg' )
PrepareMain()
PreparePB()
# .svn cleanup
for i in [ 'main/tree', 'main/resource', 'pb/tree', 'pb/resource' ]:
	cmd = 'find ' + i + ' -type d -name "\.svn" -exec rm -rf "{}" \; 2>/dev/null'
	print cmd
	os.system( cmd )
	# set file ownerships to root/admin so the setups actually work on other people's machine
	# which incidentally means that you won't be able to build software for redistribution if you don't have root
	mycmds.SimpleCommand( 'chown -R root:admin ' + i )
	# now fix permissions for group and others
	mycmds.SimpleCommand( 'chmod -R 775 ' + i )

# build the packages through the GUI thing
print 'Build the packages and press enter ( you probably need to build as root as well )'
sys.stdin.readline()

MakeImage()
