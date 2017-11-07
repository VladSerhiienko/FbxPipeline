#include <fbxppch.h>
#include <fbxpstate.h>

#define _FbxPipeline_UnsafeFileReadWrite

#if defined(_FbxPipeline_UnsafeFileReadWrite)
#include <stdio.h>
#endif

#include <experimental/filesystem>
// #include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace std {
    namespace filesystem = std::experimental::filesystem::v1;
    // namespace filesystem = std::tr2::sys;
}

std::string CurrentDirectory( ) {
    return std::filesystem::current_path( ).string( );
}

bool MakeDirectory( const char* directory ) {
    return std::filesystem::create_directory( directory );
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

std::string ReplaceSlashes( std::string path ) {
    std::replace( path.begin( ), path.end( ), '\\', '/' );
    return path;
}

std::string ReplaceExtension( const char* path, const char* extension ) {
    return std::filesystem::path( path ).replace_extension( extension ).string( );
}

std::string RealPath( std::string path ) {
    return path;
    // return std::filesystem::canonical( path ).string( );
}

std::string GetParentPath( const char* path ) {
    return std::filesystem::path( path ).parent_path( ).string( );
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

    if ( filepath && strlen( filepath ) ) {
        const std::string filename = GetFileName( filepath );
        for ( auto& searchLocation : s.searchLocations ) {
            for ( auto fileOrFolderPath : std::filesystem::directory_iterator( searchLocation ) ) {
                if ( std::filesystem::is_regular_file( fileOrFolderPath ) &&
                     fileOrFolderPath.path( ).filename( ) == filename ) {
                    return ResolveFullPath( fileOrFolderPath.path( ).string( ).c_str( ) );
                }
            }
        }
    }

    return "";
}

bool WriteFile( const char* srcFilePath, const void* data, size_t dataSize ) {
    if ( FILE* srcFile = fopen( srcFilePath, "wb" ) ) {
        auto writtenDataSize = fwrite( data, 1, dataSize, srcFile );
        fclose( srcFile );
        srcFile = nullptr;
        return writtenDataSize == dataSize;
    }

    return false;
}

std::string ReadTxtFile( const char* srcPath ) {
    const std::string srcFilePath = FindFile( srcPath );

#if defined(_FbxPipeline_UnsafeFileReadWrite)
    std::string fileBuffer;
    if ( FILE* srcFile = fopen( srcFilePath.c_str( ), "r" ) ) {
        fseek( srcFile, 0, SEEK_END );
        size_t srcImgFileSize = ftell( srcFile );
        fseek( srcFile, 0, SEEK_SET );
        fileBuffer.resize( srcImgFileSize );
        fread( &fileBuffer[ 0 ], 1, srcImgFileSize, srcFile );
        fclose( srcFile );
        srcFile = nullptr;
    }

    return fileBuffer;
#else
    /* Even though this code is safer and nicer, it's super slow. */

    if ( false == srcFilePath.empty( ) ) {
        std::ifstream filestream( srcFilePath, std::ios::binary );

        if ( filestream.good( ) )
            return std::string( std::istreambuf_iterator< char >( filestream ),
                                std::istreambuf_iterator< char >( ) );
    }

    assert( false && "Failed to open file." );
    return std::string( );
#endif
}

bool ReadBinFile( const char* srcPath, std::vector< uint8_t >& fileBuffer ) {
    const std::string srcFilePath = FindFile( srcPath );

    if ( FILE* srcFile = fopen( srcFilePath.c_str( ), "rb" ) ) {
        fseek( srcFile, 0, SEEK_END );
        size_t srcImgFileSize = ftell( srcFile );
        fseek( srcFile, 0, SEEK_SET );
        fileBuffer.resize( srcImgFileSize );
        fread( fileBuffer.data( ), 1, srcImgFileSize, srcFile );
        fclose( srcFile );
        srcFile = nullptr;
        return true;
    }

    fileBuffer.clear( );
    return false;
}

std::string convertToString( double num ) {
    std::ostringstream convert;
    convert << num;
    return convert.str( );
}

double roundOff( double n ) {
    double d = n * 100.0;
    int i = d + 0.5;
    d = (float) i / 100.0;
    return d;
}

std::string ToPrettySizeString( size_t size ) {
    static const char* SIZES[] = {"B", "KB", "MB", "GB"};
    int div = 0;
    size_t rem = 0;

    while ( size >= 1024 && div < ( sizeof SIZES / sizeof *SIZES ) ) {
        rem = ( size % 1024 );
        div++;
        size /= 1024;
    }

    double size_d = (double) size + (double) rem / 1024.0;
    std::string result = convertToString( roundOff( size_d ) ) + " " + SIZES[ div ];
    return result;
}

std::vector< uint8_t > ReadBinFile( const char* srcPath ) {
    const std::string srcFilePath = FindFile( srcPath );

#if defined(_FbxPipeline_UnsafeFileReadWrite)
    std::vector< uint8_t > fileBuffer;
    if ( FILE* srcFile = fopen( srcFilePath.c_str( ), "rb" ) ) {
        fseek( srcFile, 0, SEEK_END );
        size_t srcImgFileSize = ftell( srcFile );
        fseek( srcFile, 0, SEEK_SET );
        fileBuffer.resize( srcImgFileSize );
        fread( fileBuffer.data( ), 1, srcImgFileSize, srcFile );
        fclose( srcFile );
        srcFile = nullptr;
    }

    return fileBuffer;
#else
    /* Even though this code is safer and nicer, it's super slow. */

    if ( false == srcFilePath.empty( ) ) {
        std::ifstream filestream( srcFilePath, std::ios::binary );

        if ( filestream.good( ) )
            return std::vector< uint8_t >( std::istreambuf_iterator< char >( filestream ),
                                           std::istreambuf_iterator< char >( ) );
    }

    assert( false && "Failed to open file." );
    return std::vector< uint8_t >( );
#endif
}

void InitializeSeachLocations( ) {
    auto& s = apemode::Get( );
    auto searchLocations = s.options[ "e" ].as< std::vector< std::string > >( );

    std::string inputFile = s.options[ "i" ].as< std::string >( );
    ReplaceSlashes( inputFile );

    const std::string fbmDirectory = ReplaceSlashes( ReplaceExtension( inputFile.c_str( ), "fbm" ) );
    searchLocations.push_back( fbmDirectory + "/**" );

    const std::string inputDirectory = ReplaceSlashes( GetParentPath( inputFile.c_str( ) ) );
    if ( DirectoryExists( inputDirectory.c_str( ) ) ) {
        searchLocations.push_back( inputDirectory + "/**" );

        /**
         * Sketchfab archives usually have a file structure like:
         *   > /c/path/to/model/ModelName/source/model.fbx
         *   > /c/path/to/model/ModelName/textures/diffuse.png, ...
         **/

        std::string textureDirectorySketchfab = ReplaceSlashes( RealPath( ( inputDirectory + "/../textures" ) ) );
        if ( DirectoryExists( textureDirectorySketchfab.c_str( ) ) ) {
            searchLocations.push_back( textureDirectorySketchfab + "/**" );
        } else {
            /* Model file could be additionally archived, and when unarchived could be placed in the directory. */
            textureDirectorySketchfab = ReplaceSlashes( RealPath( ( inputDirectory + "/../../textures" ) ) );
            if ( DirectoryExists( textureDirectorySketchfab.c_str( ) ) ) {
                searchLocations.push_back( textureDirectorySketchfab + "/**" );
            }
        }
    }

    std::set< std::string > searchDirectories;

    for ( auto searchDirectory : searchLocations ) {
        bool addSubDirectories = false;

        if ( searchDirectory.size( ) > 4 ) {
            const size_t ds = searchDirectory.size( );
            addSubDirectories |= searchDirectory.compare( ds - 3, 3, "\\**" ) == 0;
            addSubDirectories |= searchDirectory.compare( ds - 3, 3, "/**" ) == 0;

            if ( addSubDirectories )
                searchDirectory = searchDirectory.substr( 0, searchDirectory.size( ) - 2 );
        }

        if ( DirectoryExists( searchDirectory.c_str( ) ) ) {
            if ( !addSubDirectories ) {
                searchDirectories.insert( ResolveFullPath( searchDirectory.c_str( ) ) );
            } else {
                const std::string resolvedSearchDirectory = ResolveFullPath( searchDirectory.c_str( ) );
                searchDirectories.insert( resolvedSearchDirectory );

                for ( auto& searchDirectoryIt : std::filesystem::recursive_directory_iterator( resolvedSearchDirectory ) ) {
                    if ( std::filesystem::is_directory(searchDirectoryIt.path( ) ) )
                        searchDirectories.insert( ResolveFullPath( searchDirectoryIt.path( ).string( ).c_str( ) ) );
                }
            }
        }
    }

    if ( false == searchDirectories.empty( ) ) {
        s.searchLocations.assign( searchDirectories.begin( ), searchDirectories.end( ) );

        s.console->info( "Search locations:" );
        for ( auto& searchLocation : s.searchLocations ) {
            s.console->info( "\t> {}", searchLocation );
        }

        const auto& embedFilePatterns = s.options[ "m" ].as< std::vector< std::string > >( );
        for ( auto& embedFilePattern : embedFilePatterns ) {
            std::regex embedFilePatternRegex( embedFilePattern );
            for ( auto& searchLocation : s.searchLocations ) {
                for ( auto fileOrFolderPath : std::filesystem::directory_iterator( searchLocation ) ) {
                    if ( std::filesystem::is_regular_file( fileOrFolderPath ) &&
                         std::regex_match( fileOrFolderPath.path( ).string( ), embedFilePatternRegex ) ) {
                        s.embedQueue.insert( ResolveFullPath( fileOrFolderPath.path( ).string( ).c_str( ) ) );
                    }
                }
            }
        }
    }
}