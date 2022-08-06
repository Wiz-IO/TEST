# https://docs.platformio.org/en/latest/scripting/examples/external_sources.html

from os.path import join

Import('env')
#env = DefaultEnvironment()

OBJ_DIR = join( "$BUILD_DIR", 'EXTERNAL' )
SDK_DIR = join( env.PioPlatform().get_package_dir("framework-wizio-pico"), 'SDK' ) 

#''' NOT WORK for POST:SCRYPT
env.BuildSources( 
    # add lwip/tftp from framework https://github.com/Wiz-IO/framework-wizio-pico/tree/main/SDK/lib/lwip/src/apps/tftp
    join( OBJ_DIR, 'lwip', 'tftp' ),                      
    join( SDK_DIR, 'lib', 'lwip', 'src', 'apps', 'tftp' ),   
    "+<*>"
)
#'''


''' WORK for PRE and POST script
libs = []
libs.append( env.BuildLibrary(
    join( OBJ_DIR, 'LIB', 'tftp' ),                      
    join( SDK_DIR, 'lib', 'lwip', 'src', 'apps', 'tftp' ),   
))
env.Prepend(LIBS=libs)
'''


