# https://docs.platformio.org/en/latest/scripting/examples/external_sources.html

from os.path import join

Import('env')
#env = DefaultEnvironment()

OBJ_DIR = join( "$BUILD_DIR", 'EXTERNAL' )
SDK_DIR = join( env.PioPlatform().get_package_dir("framework-wizio-pico"), 'SDK' ) 

#''' NOT WORK as POST:SCRYPT
env.BuildSources( 
    # add lwip/tftp from framework https://github.com/Wiz-IO/framework-wizio-pico/tree/main/SDK/lib/lwip/src/apps/tftp
    join( OBJ_DIR, 'lwip', 'tftp' ),                      
    join( SDK_DIR, 'lib', 'lwip', 'src', 'apps', 'tftp' ),   
    "+<*>"
)
env.BuildSources( # just for test, code from project folder 
    join(OBJ_DIR, "code"),
    join("$PROJECT_DIR", "code"),
    "+<*>"
)
#'''


''' WORK as PRE and POST script
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
