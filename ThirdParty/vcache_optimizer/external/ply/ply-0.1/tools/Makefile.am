bin_PROGRAMS = plyheader ply2ply ply2obj ply2raw
plyheader_SOURCES = plyheader.cpp
ply2ply_SOURCES = ply2ply.cpp
ply2obj_SOURCES = ply2obj.cpp
ply2raw_SOURCES = ply2raw.cpp
LDADD = $(top_builddir)/ply/libply.la
INCLUDES = -I$(top_srcdir)
AM_CXXFLAGS = $(BOOST_CPPFLAGS) -Wall -ansi -pedantic
