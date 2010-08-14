# -*- mode: python -*-
import sys, os, string, time, commands, re, pickle, StringIO, popen2, commands, pdb, zipfile
import SCons

# need an Environment and a matching buffered_spawn API .. encapsulate
class idBuffering:

	def buffered_spawn( self, sh, escape, cmd, args, env ):
		stderr = StringIO.StringIO()
		stdout = StringIO.StringIO()
		command_string = ''
		for i in args:
			if ( len( command_string ) ):
				command_string += ' '
			command_string += i
		try:
			retval = self.env['PSPAWN']( sh, escape, cmd, args, env, stdout, stderr )
		except OSError, x:
			if x.errno != 10:
				raise x
			print 'OSError ignored on command: %s' % command_string
			retval = 0
		print command_string
		sys.stdout.write( stdout.getvalue() )
		sys.stderr.write( stderr.getvalue() )
		return retval		

class idSetupBase:
	
	def SimpleCommand( self, cmd ):
		print cmd
		ret = commands.getstatusoutput( cmd )
		if ( len( ret[ 1 ] ) ):
			sys.stdout.write( ret[ 1 ] )
			sys.stdout.write( '\n' )
		if ( ret[ 0 ] != 0 ):
			raise 'command failed'
		return ret[ 1 ]

	def TrySimpleCommand( self, cmd ):
		print cmd
		ret = commands.getstatusoutput( cmd )
		sys.stdout.write( ret[ 1 ] )

	def M4Processing( self, file, d ):
		file_out = file[:-3]
		cmd = 'm4 '
		for ( key, val ) in d.items():
			cmd += '--define=%s="%s" ' % ( key, val )
		cmd += '%s > %s' % ( file, file_out )
		self.SimpleCommand( cmd )	

def checkLDD( target, source, env ):
	file = target[0]
	if (not os.path.isfile(file.abspath)):
		print('ERROR: CheckLDD: target %s not found\n' % target[0])
		Exit(1)
	( status, output ) = commands.getstatusoutput( 'ldd -r %s' % file )
	if ( status != 0 ):
		print 'ERROR: ldd command returned with exit code %d' % ldd_ret
		os.system( 'rm %s' % target[ 0 ] )
		sys.exit(1)
	lines = string.split( output, '\n' )
	have_undef = 0
	for i_line in lines:
		#print repr(i_line)
		regex = re.compile('undefined symbol: (.*)\t\\((.*)\\)')
		if ( regex.match(i_line) ):
			symbol = regex.sub('\\1', i_line)
			try:
				env['ALLOWED_SYMBOLS'].index(symbol)
			except:
				have_undef = 1
	if ( have_undef ):
		print output
		print "ERROR: undefined symbols"
		os.system('rm %s' % target[0])
		sys.exit(1)

def SharedLibrarySafe( env, target, source ):
	ret = env.SharedLibrary( target, source )
	env.AddPostAction( ret, checkLDD )
	return ret

def NotImplementedStub( ):
	print 'Not Implemented'
	sys.exit( 1 )

# --------------------------------------------------------------------

# get a clean error output when running multiple jobs
def SetupBufferedOutput( env ):
	buf = idBuffering()
	buf.env = env
	env['SPAWN'] = buf.buffered_spawn

# setup utilities on an environement
def SetupUtils( env ):
	env.SharedLibrarySafe = SharedLibrarySafe
	try:
		import SDK
		env.BuildSDK = SDK.BuildSDK
	except:
		env.BuildSDK = NotImplementedStub
	try:
		import Setup
		setup = Setup.idSetup()
		env.BuildSetup = setup.BuildSetup
	except:
		env.BuildSetup = NotImplementedStub

def BuildList( s_prefix, s_string ):
	s_list = string.split( s_string )
	for i in range( len( s_list ) ):
		s_list[ i ] = s_prefix + '/' + s_list[ i ]
	return s_list
