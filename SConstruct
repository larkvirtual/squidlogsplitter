env = Environment()
env.Replace(CCFLAGS = '-O3 --omit-frame-pointer -pipe')
#env.Append(LINKFLAGS = '-static')

DEBUG = ARGUMENTS.get('DEBUG', 0)
if int(DEBUG):
    env.Replace(CCFLAGS = '-g -ggdb -Wall -W -O0 -ansi -pedantic -pipe -DDEBUG')
    env.Replace(LINKFLAGS = '')

Export('env')

env.SConscript('src/SConscript')

opts = Options('custom.py')
opts.Add('DEBUG', 'Set to 1 to build for debug', 0)
Help(opts.GenerateHelpText(env))
