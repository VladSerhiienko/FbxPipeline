
#include <fbxppch.h>
#include <fbxpstate.h>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>

namespace py = pybind11;

namespace apemode {
    void RegisterExtension( std::function< void( apemode::State*, std::string ) > func ) {
        auto& s = apemode::Get( );
        s.console->info( "Registering extension function ..." );
        s.extensions.push_back( func );
    }
} // namespace apemode

PYBIND11_MODULE( FbxPipeline, m ) {
    m.doc( ) = "python extension base for FbxPipeline";

    py::class_< apemode::State > state( m, "State" );
    m.def( "RegisterExtension", &apemode::RegisterExtension );
}

bool        FileExists( const char* path );
std::string FileExtension( const char* path );
std::string ToPrettySizeString( size_t size );
bool        ReadTxtFile( const char* srcPath, std::string& fileBuffer, bool findFile );

void RunExtensionsOnFinalize( ) {
    auto& s = apemode::Get( );

    auto& scriptFiles  = s.options[ "script-file" ].as< std::vector< std::string > >( );
    auto& scriptInputs = s.options[ "script-input" ].as< std::vector< std::string > >( );

    bool bContainsPythonExtensions = false;
    for ( auto& scriptFile : scriptFiles ) {
        if ( FileExtension( scriptFile.c_str( ) ) == ".py" ) {
            s.console->info( "Found a script with .py extension" );
            bContainsPythonExtensions = true;
            break;
        }
    }

    if ( bContainsPythonExtensions ) {

        // Start the interpreter and keep it alive
        s.console->info( "Creating python interpreter ..." );
        py::scoped_interpreter guard{};

        // Load FbxPipeline module from .pyd in current directory.
        // TODO: _d suffix is not needed for debug builds, isn't it?
        s.console->info( "Importing python FbxPipeline module ..." );
        py::module thisModule = py::module::import( "FbxPipeline" );

        std::string pythonFileContent;
        for ( auto& scriptFile : scriptFiles ) {
            if ( FileExtension( scriptFile.c_str( ) ) == ".py" ) {
                if ( ReadTxtFile( scriptFile.c_str( ), pythonFileContent, true ) ) {

                    try {
                        s.console->info( "Executing python script \"{}\" (size: {}) ... ", scriptFile, ToPrettySizeString( scriptFile.size( ) ) );
                        py::exec( pythonFileContent.c_str( ) );
                    } catch ( py::error_already_set e ) {
                        s.console->error( "Failed to execute \"{}\": error_already_set\n{}", scriptFile, e.what( ) );
                    } catch ( std::runtime_error e ) {
                        s.console->error( "Failed to execute \"{}\": runtime_error\n{}", scriptFile, e.what( ) );
                    } catch ( std::exception e ) {
                        s.console->error( "Failed to execute \"{}\": exception\n{}", scriptFile, e.what( ) );
                    } catch ( ... ) {
                        s.console->error( "Failed to execute \"{}\": unknown error", scriptFile );
                    }
                }
            }
        }

        for ( auto& e : s.extensions ) {
            for ( auto& i : scriptInputs ) {

                try {
                    e( &s, i );
                } catch ( std::runtime_error e ) {
                    s.console->error( "Failed to run extension: runtime_error\n{}", e.what( ) );
                } catch ( std::exception e ) {
                    s.console->error( "Failed to run extension: exception\n{}\n", e.what( ) );
                } catch ( ... ) {
                    s.console->error( "Failed to run extension: unknown error" );
                }
            }
        }
    }

    // TODO: Other interpreters.
    //       JS, lua, ...
}