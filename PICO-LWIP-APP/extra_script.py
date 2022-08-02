# https://docs.platformio.org/en/latest/scripting/examples/external_sources.html

#Import('env')
env = DefaultEnvironment()
from os.path import join

print('--- ADD MODULES ---')
OBJ_DIR = join( "$BUILD_DIR", env.platform )
SDK_DIR = join( env.framework_dir, env.sdk )


print( join( OBJ_DIR, 'lwip', 'tftp' ) )
# $BUILD_DIR\sdk\lwip\tftp
print( join( SDK_DIR, 'lib', 'lwip', 'src', 'apps', 'tftp' ) )   
# file exist C:\Users\1124\.platformio\packages\framework-wizio-pico\SDK\lib\lwip\src\apps\tftp

env.BuildSources( # add lwip/tftp
    join( OBJ_DIR, 'lwip', 'tftp' ),                         
    join( SDK_DIR, 'lib', 'lwip', 'src', 'apps', 'tftp' ),   
    "+<*>"
)

env.BuildSources( # just for test from project
    join(OBJ_DIR, "code"),
    join("$PROJECT_DIR", "code"),
    "+<*>"
)

#env.Append(CPPDEFINES='WIZIO_TEST') # WORK

print('--- END MODULES ---')