#include <fbxppch.h>
#include <fbxpstate.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace std {
    namespace filesystem = std::tr2::sys;
}

std::string CurrentDirectory( ) {
    return std::filesystem::current_path( ).string( );
}

std::string GetFileName( const char* filePath ) {
    return std::filesystem::path( filePath ).filename( ).string( );
}

bool PathExists( const char* path ) {
    return std::filesystem::exists( path );
}

bool DirectoryExists( const char* directoryPath ) {
    return std::filesystem::is_directory( directoryPath );
}

bool FileExists( const char* filePath ) {
    return std::filesystem::is_regular_file( filePath );
}

std::string& ReplaceSlashes( std::string& path ) {
    std::replace( path.begin( ), path.end( ), '\\', '/' );
    return path;
}

std::string& RealPath( std::string& path ) {
    return path = std::filesystem::canonical( path ).string( );
}

std::string ResolveFullPath( const char* path ) {
    if ( std::filesystem::is_directory( path ) ) {
        const size_t lastIndex = strlen( path ) - 1;
        if ( path[ lastIndex ] != '\\' && path[ lastIndex ] != '/' )
            return ReplaceSlashes( RealPath( std::filesystem::absolute( path ).string( ) ) ) + "/";
    }

    return ReplaceSlashes( RealPath( std::filesystem::absolute( path ).string( ) ) );
}

std::string FindFile( const char* filepath ) {
    auto& s = apemode::Get( );

    assert( filepath && strlen( filepath ) );
    const std::string filename = GetFileName( filepath );

    for ( auto& searchLocation : s.searchLocations ) {
        for ( auto fileOrFolderPath : std::filesystem::directory_iterator( searchLocation ) ) {
            if ( std::filesystem::is_regular_file( fileOrFolderPath ) &&
                 fileOrFolderPath.path( ).filename( ) == filename ) {
                return ResolveFullPath( fileOrFolderPath.path( ).string( ).c_str( ) );
            }
        }
    }

    return "";
}

std::vector< uint8_t > ReadFile( const char* filepath ) {
    const std::string fullpath = FindFile( filepath );

    if ( false == fullpath.empty( ) ) {
        std::ifstream filestream( fullpath, std::ios::binary );

        if ( filestream.good( ) )
            return std::vector< uint8_t >( std::istreambuf_iterator< char >( filestream ),
                                           std::istreambuf_iterator< char >( ) );
    }

    assert( false && "Failed to open file." );
    return std::vector< uint8_t >( );
}

void InitializeSeachLocations( ) {
    auto& s = apemode::Get( );
    auto& sl = s.options[ "e" ].as< std::vector< std::string > >( );

    std::set< std::string > searchDirectories;
    for ( auto d : sl ) {
        bool addSubDirectories = false;

        if ( d.size( ) > 4 ) {
            const size_t ds = d.size( );
            addSubDirectories |= d.compare( ds - 3, 3, "\\**" ) == 0;
            addSubDirectories |= d.compare( ds - 3, 3, "/**" ) == 0;

            if ( addSubDirectories )
                d = d.substr( 0, d.size( ) - 2 );
        }

        if ( DirectoryExists( d.c_str( ) ) ) {
            if ( !addSubDirectories ) {
                searchDirectories.insert( ResolveFullPath( d.c_str( ) ) );
            } else {
                const std::string dd = ResolveFullPath( d.c_str( ) );
                searchDirectories.insert( dd );

                for ( auto& rd : std::filesystem::recursive_directory_iterator( dd ) ) {
                    if ( std::filesystem::is_directory( rd.path( ) ) )
                        searchDirectories.insert( ResolveFullPath( rd.path( ).string( ).c_str( ) ) );
                }
            }
        }
    }

    s.searchLocations.assign( searchDirectories.begin( ), searchDirectories.end( ) );
    s.console->info( "Search locations:" );
    for ( auto& l : s.searchLocations ) {
        s.console->info( "\t{}", l );
    }

    auto& ef = s.options[ "m" ].as< std::vector< std::string > >( );
    for ( auto f : ef ) {
        std::regex pattern( f );
        for ( auto& searchLocation : s.searchLocations ) {
            for ( auto fileOrFolderPath : std::filesystem::directory_iterator( searchLocation ) ) {
                if ( std::filesystem::is_regular_file( fileOrFolderPath ) &&
                     std::regex_match( fileOrFolderPath.path( ).string( ), pattern ) ) {
                    s.embedQueue.insert( ResolveFullPath( fileOrFolderPath.path( ).string( ).c_str( ) ) );
                }
            }
        }
    }
}