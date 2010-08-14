#!/usr/bin/env python
# easily spawn processes, with on screen trace and error checking

import sys, commands

def SimpleCommand( cmd ):
    print cmd
    ret = commands.getstatusoutput( cmd )
    if ( len( ret[ 1 ] ) ):
        sys.stdout.write( ret[ 1 ] )
        sys.stdout.write( '\n' )
    if ( ret[ 0 ] != 0 ):
        raise 'command failed'
    return ret[ 1 ]

def TrySimpleCommand( cmd ):
    print cmd
    ret = commands.getstatusoutput( cmd )
    sys.stdout.write( ret[ 1 ] )
    return ret[ 0 ]

def SilentCommand( cmd ):
	ret = commands.getstatusoutput( cmd )
	if ( ret[ 0 ] != 0 ):
		sys.stdout.write( ret[ 1 ] )
		sys.stdout.write( '\n' )
		raise 'command failed'
	# NOTE: the appended '\n' is quite important, makes it so we data is actually equivalent to a shell 'cmd |' / 'cmd > ~/out'
	return ret[ 1 ] + '\n'
