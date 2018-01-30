
#include <fbxppch.h>
#include <fbxpstate.h>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
namespace py = pybind11;

PYBIND11_MODULE( fbxppyextension, m ) {
    m.doc( ) = "python extension for fbx pipeline";

    py::class_< apemode::State > state( m, "State" );

    py::class_< apemode::ExtensionBase > pyextension( m, "ExtensionBase" );
    pyextension.def( "run", &apemode::ExtensionBase::run );
}

