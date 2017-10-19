lib_LTLIBRARIES = libply.la
libply_la_SOURCES = ../ply.hpp byte_order.hpp io_operators.hpp ply_parser.cpp config.hpp ply.hpp ply_parser.hpp
INCLUDES = -I$(top_srcdir)
library_includedir = $(includedir)/ply
library_include_HEADERS = byte_order.hpp io_operators.hpp config.hpp ply.hpp ply_parser.hpp
libply_la_DEPENDENCIES = ../ply.hpp
AM_CXXFLAGS = $(BOOST_CPPFLAGS) -Wall -ansi -pedantic
