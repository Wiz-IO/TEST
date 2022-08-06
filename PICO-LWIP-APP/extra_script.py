# https://docs.platformio.org/en/latest/scripting/examples/external_sources.html

from os.path import join

Import('env')
#env = DefaultEnvironment()

print('--- ADD MODULES ---')

OBJ_DIR = join( "$BUILD_DIR", 'EXTERNAL' )

# PRE:SCRYPT - I have uninitialized platform variables
# env.sdk ... 'SDK' is folder of last sdk version, can be changed with old sdk backup 'SDK120, 131 ...' 

#SDK_DIR = join( env.framework_dir, env.sdk )
SDK_DIR = join( env.PioPlatform().get_package_dir("framework-wizio-pico"), 'SDK' ) 


#''' NOT WORK as POST:SCRYPT
env.BuildSources( # add lwip/tftp 
    join( OBJ_DIR, 'lwip', 'tftp' ),                      
    join( SDK_DIR, 'lib', 'lwip', 'src', 'apps', 'tftp' ),   
    "+<*>"
)
env.BuildSources( # just for test, code from project 
    join(OBJ_DIR, "code"),
    join("$PROJECT_DIR", "code"),
    "+<*>"
)
#'''


''' WORK as PRE & POST
libs = []
libs.append( env.BuildLibrary(
    join( OBJ_DIR, 'LIB', 'tftp' ),                      
    join( SDK_DIR, 'lib', 'lwip', 'src', 'apps', 'tftp' ),   
))
libs.append( env.BuildLibrary( # just for test, code from project
    join(OBJ_DIR, "LIB", "code"),
    join("$PROJECT_DIR", "code"),
))
env.Prepend(LIBS=libs)
#'''


#print('--- END MODULES ---')
#print( env.Dump() )