env = Environment(CCFLAGS = '-g3 -O0 -std=gnu99 -Wall', tools=['default','packaging'])
env.ParseConfig('pkg-config --cflags --libs gtk+-2.0 sqlite3')

srcs = Split(
"""
data.c
main.c
sqlitemodel.c
"""
)

cslov = env.Program(target = 'cslov', source = srcs)

files = env.Install(dir = '/bin/', source = cslov)

rpm = env.Package(NAME = 'cslov', 
	VERSION = '1.0', 
	PACKAGEVERSION = 1, 
	PACKAGETYPE = 'rpm', 
	LICENSE = 'gpl', 
	SUMMARY = 'cslov', 
	X_RPM_GROUP = 'System/Foo', 
	DESCRIPTION = 'cslov rpm', 
	source = [files])
	
env.Alias('rpm', rpm) 

Default(cslov)