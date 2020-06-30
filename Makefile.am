ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = \
	LICENSE CHANGELOG.md README.md \
	.gitignore

MAINTAINERCLEANFILES = statedata.h
BUILT_SOURCES = genstatedata

bin_PROGRAMS = genstatedata sfrequency ufrequency urun2

statedata.h : genstatedata.cc
	./genstatedata > statedata.h

# @date 2020-06-30 19:20:55
genstatedata_SOURCES = genstatedata.cc

# @date 2020-06-26 00:06:58
sfrequency_SOURCES = sfrequency.c

# @date 2020-06-29 14:07:33
ufrequency_SOURCES = ufrequency.c

# @date 2020-06-30 00:26:04
urun2_SOURCES = urun2.c statedata.h
