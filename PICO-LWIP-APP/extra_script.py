# https://docs.platformio.org/en/latest/scripting/examples/external_sources.html

from os.path import join

Import('env')
#env = DefaultEnvironment()

print('--- ADD MODULES ---')

OBJ_DIR = join( "$BUILD_DIR", env.platform )
SDK_DIR = join( env.framework_dir, env.sdk )

#print( join( OBJ_DIR, 'lwip', 'tftp' ) )
#print( join( SDK_DIR, 'lib', 'lwip', 'src', 'apps', 'tftp' ) ) # PATH is OK   

''' NOT WORK
env.BuildSources( # add lwip/tftp 
    join( OBJ_DIR, 'lwip', 'tftp' ),                      
    join( SDK_DIR, 'lib', 'lwip', 'src', 'apps', 'tftp' ),   
    "+<*>"
)
'''

''' NOT WORK
env.BuildSources( # just for test, code from project 
    join(OBJ_DIR, "code"),
    join("$PROJECT_DIR", "code"),
    "+<*>"
)
'''

#''' WORK
libs = []
libs.append( env.BuildLibrary(
    join( OBJ_DIR, 'MODULES', 'tftp' ),                      
    join( SDK_DIR, 'lib', 'lwip', 'src', 'apps', 'tftp' ),   
))
libs.append( env.BuildLibrary( # just for test, code from project
    join(OBJ_DIR, "MODULES", "code"),
    join("$PROJECT_DIR", "code"),
))
env.Prepend(LIBS=libs)
#'''



#env.Append(CPPDEFINES='WIZIO_TEST') # WORK

# executed before Linking
#env.AddPreAction( "$BUILD_DIR/${PROGNAME}.elf", env.VerboseAction(" ".join([ "notepad" ]), "Open $PROJECT_DIR ") ) # WORK



print('--- END MODULES ---')
#print( env.Dump() )