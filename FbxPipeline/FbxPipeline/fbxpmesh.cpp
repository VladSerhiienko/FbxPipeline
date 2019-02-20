#include <fbxppch.h>
#include <fbxpstate.h>
#include <fbxpnorm.h>
#include <map>
#include <array>

#ifdef ERROR
#undef ERROR
#endif

#include <draco/mesh/mesh.h>
#include <draco/mesh/mesh_cleanup.h>
#include <draco/mesh/triangle_soup_mesh_builder.h>
#include <draco/compression/encode.h>
#include <draco/compression/expert_encode.h>

namespace mathfu {
    using dvec2 = mathfu::Vector< double, 2 >;
    using dvec3 = mathfu::Vector< double, 3 >;
    using dvec4 = mathfu::Vector< double, 4 >;
    using dmat3 = mathfu::Matrix< double, 3, 3 >;
    using dmat4 = mathfu::Matrix< double, 4, 4 >;
    using dquat = mathfu::Quaternion< double >;

    vec2 to_float( dvec2 v ) {
        return {(float) v[ 0 ], (float) v[ 1 ]};
    }
    vec3 to_float( dvec3 v ) {
        return {(float) v[ 0 ], (float) v[ 1 ], (float) v[ 2 ]};
    }
    vec4 to_float( dvec4 v ) {
        return {(float) v[ 0 ], (float) v[ 1 ], (float) v[ 2 ], (float) v[ 3 ]};
    }
} // namespace mathfu

/**
 * Helper structure to assign vertex property values
 **/
struct StaticVertex {
    mathfu::dvec3 position;
    mathfu::dvec3 normal;
    mathfu::dvec4 tangent;
    mathfu::dvec4 color;
    mathfu::dvec2 texCoords;
    int           controlPointIndex;
};

template < typename T >
void AssertValidFloat( T value ) {
    assert( !isnan( value ) );
    assert( !isinf( value ) );
}

template < typename T >
void AssertValidVec3( T values ) {
    AssertValidFloat( values[ 0 ] );
    AssertValidFloat( values[ 1 ] );
    AssertValidFloat( values[ 2 ] );
}

bool CalculateTangentsNoUVs( StaticVertex* vertices, size_t vertexCount ) {
    std::vector< mathfu::dvec3 > tan;
    tan.resize( vertexCount * 2 );

    auto tan1 = tan.data( );
    auto tan2 = tan1 + vertexCount;

    for ( size_t i = 0; i < vertexCount; i += 3 ) {
        const StaticVertex v0 = vertices[ i + 0 ];
        const StaticVertex v1 = vertices[ i + 1 ];
        const StaticVertex v2 = vertices[ i + 2 ];

        mathfu::dvec3 e1 = v1.position - v0.position;
        mathfu::dvec3 e2 = v2.position - v0.position;
        e1.Normalize( );
        e2.Normalize( );

        AssertValidVec3( e1 );
        AssertValidVec3( e2 );

        tan1[ i + 0 ] = e1;
        tan1[ i + 1 ] = e1;
        tan1[ i + 2 ] = e1;

        tan2[ i + 0 ] = e2;
        tan2[ i + 1 ] = e2;
        tan2[ i + 2 ] = e2;
    }

    bool result = true;

    for ( size_t i = 0; i < vertexCount; i += 1 ) {
        StaticVertex& v = vertices[ i ];

        v.tangent   = mathfu::kZeros4d;
        v.tangent.w = 1;

        const mathfu::dvec3 n = v.normal.Normalized( );

        mathfu::dvec3 t = tan1[ i ];
        AssertValidVec3( n );
        AssertValidVec3( t );

        const bool isOk = n.Length( ) && t.Length( );
        result &= isOk;
        if ( isOk ) {
            mathfu::dvec3 tt = mathfu::normalize( t - n * mathfu::dot( n, t ) );
            AssertValidVec3( tt );

            v.tangent[ 0 ] = tt.x;
            v.tangent[ 1 ] = tt.y;
            v.tangent[ 2 ] = tt.z;
            v.tangent[ 3 ] = ( mathfu::dot( mathfu::cross( n, t ), tan2[ i ] ) < 0 ) ? -1.0f : 1.0f;
        }
    }

    return result;
}

/**
 * Helper function to calculate tangents when the tangent element layer is missing.
 * Can be used in multiple threads.
 * http://gamedev.stackexchange.com/a/68617/39505
 **/
bool CalculateTangents( StaticVertex* vertices, size_t vertexCount ) {
    std::vector< mathfu::dvec3 > tan;
    tan.resize( vertexCount * 2 );

    auto tan1 = tan.data( );
    auto tan2 = tan1 + vertexCount;

    for ( size_t i = 0; i < vertexCount; i += 3 ) {
        const StaticVertex v0 = vertices[ i + 0 ];
        const StaticVertex v1 = vertices[ i + 1 ];
        const StaticVertex v2 = vertices[ i + 2 ];

        const auto x1 = v1.position[ 0 ] - v0.position[ 0 ];
        const auto x2 = v2.position[ 0 ] - v0.position[ 0 ];
        const auto y1 = v1.position[ 1 ] - v0.position[ 1 ];
        const auto y2 = v2.position[ 1 ] - v0.position[ 1 ];
        const auto z1 = v1.position[ 2 ] - v0.position[ 2 ];
        const auto z2 = v2.position[ 2 ] - v0.position[ 2 ];

        const auto s1 = v1.texCoords[ 0 ] - v0.texCoords[ 0 ];
        const auto s2 = v2.texCoords[ 0 ] - v0.texCoords[ 0 ];
        const auto t1 = v1.texCoords[ 1 ] - v0.texCoords[ 1 ];
        const auto t2 = v2.texCoords[ 1 ] - v0.texCoords[ 1 ];

        const auto r = 1.0f / ( ( s1 * t2 - s2 * t1 ) + std::numeric_limits< float >::epsilon( ) );
        const mathfu::dvec3 sdir( ( t2 * x1 - t1 * x2 ) * r, ( t2 * y1 - t1 * y2 ) * r, ( t2 * z1 - t1 * z2 ) * r );
        const mathfu::dvec3 tdir( ( s1 * x2 - s2 * x1 ) * r, ( s1 * y2 - s2 * y1 ) * r, ( s1 * z2 - s2 * z1 ) * r );

        AssertValidVec3( sdir );
        AssertValidVec3( tdir );

        tan1[ i + 0 ] = sdir;
        tan1[ i + 1 ] = sdir;
        tan1[ i + 2 ] = sdir;

        tan2[ i + 0 ] = tdir;
        tan2[ i + 1 ] = tdir;
        tan2[ i + 2 ] = tdir;
    }

    bool result = true;

    for ( size_t i = 0; i < vertexCount; i += 1 ) {
        StaticVertex& v = vertices[ i ];

        v.tangent   = mathfu::kZeros4d;
        v.tangent.w = 1;

        mathfu::dvec3 n = v.normal;
        mathfu::dvec3 t = tan1[ i ];
        AssertValidVec3( n );
        AssertValidVec3( t );

        const bool isOk = n.LengthSquared() && t.LengthSquared();
        result &= isOk;
        if ( isOk ) {
            n = n.Normalized( );
            mathfu::dvec3 tt = mathfu::normalize( t - n * mathfu::dot( n, t ) );
            AssertValidVec3( tt );

            v.tangent[ 0 ] = tt.x;
            v.tangent[ 1 ] = tt.y;
            v.tangent[ 2 ] = tt.z;
            v.tangent[ 3 ] = ( mathfu::dot( mathfu::cross( n, t ), tan2[ i ] ) < 0 ) ? -1.0f : 1.0f;
        }
    }

    return result;
}

/**
 * Calculate normals for the faces (does not weight triangles).
 * Fast and usable results, however incorrect.
 * TODO: Implement vertex normal calculations.
 **/
void CalculateFaceNormals( StaticVertex* vertices, size_t vertexCount ) {
    for ( size_t i = 0; i < vertexCount; i += 3 ) {
        StaticVertex& v0 = vertices[ i + 0 ];
        StaticVertex& v1 = vertices[ i + 1 ];
        StaticVertex& v2 = vertices[ i + 2 ];

        const mathfu::dvec3 p0( v0.position );
        const mathfu::dvec3 p1( v1.position );
        const mathfu::dvec3 p2( v2.position );
        mathfu::dvec3       n( mathfu::cross( p1 - p0, p2 - p0 ) );

        AssertValidVec3( p0 );
        AssertValidVec3( p1 );
        AssertValidVec3( p2 );
        AssertValidVec3( n );

        if ( n.LengthSquared( ) > std::numeric_limits< float >::epsilon( ) ) {
            const mathfu::dvec3 nn( n.Normalized( ) );
            AssertValidVec3( n );
            n = nn;
        }

        v0.normal[ 0 ] = n.x;
        v0.normal[ 1 ] = n.y;
        v0.normal[ 2 ] = n.z;

        v1.normal[ 0 ] = n.x;
        v1.normal[ 1 ] = n.y;
        v1.normal[ 2 ] = n.z;

        v2.normal[ 0 ] = n.x;
        v2.normal[ 1 ] = n.y;
        v2.normal[ 2 ] = n.z;
    }
}

/**
 * Produces mesh subsets and subset indices.
 * A subset is a structure for mapping material index to a polygon range to allow a single mesh to
 * be rendered using multiple materials.
 * The usage could be: 1) render polygon range [ 0, 12] with 1st material.
 *                     2) render polygon range [12, 64] with 2nd material.
 *                     * range is [base index; index count]
 *
 * @param indices The indices of the mesh that will be used to draw the mesh with multiple materials.
 * @param subsets The ranges of the vertex indices for each material of the node.
 * @param subsetPolies A mapping of material indices to polygon ranges (useful for knowing the basic structure).
 * @return True on success.
 **/
bool GetSubsets( FbxMesh* pMeshFb, std::vector< apemodefb::SubsetFb >& subsetsFb ) {

    auto& s = apemode::State::Get( );
    s.console->info("Mesh \"{}\" has {} material(s) assigned.", pMeshFb->GetNode( )->GetName( ), pMeshFb->GetNode( )->GetMaterialCount( ) );

    subsetsFb.clear( );

    /* No submeshes */
    if ( pMeshFb->GetNode( )->GetMaterialCount( ) == 0 ) {
        return false;
    }

    /* Single submesh */
    if ( pMeshFb->GetNode( )->GetMaterialCount( ) == 1 ) {
        subsetsFb.emplace_back( 0, 0, pMeshFb->GetPolygonCount( ) * 3 );
        return true;
    }

    /* Print materials attached to a node. */
    for ( int k = 0; k < pMeshFb->GetNode( )->GetMaterialCount( ); ++k ) {
        s.console->info( "\tMaterial #{} - \"{}\".", k, pMeshFb->GetNode( )->GetMaterial( k )->GetName( ) );
    }

    struct MaterialMappingItem {
        uint32_t materialIndex = 0;
        uint32_t polygonIndex = 0;
    };

    std::set< uint32_t > materialIndicesInUse;
    std::vector< MaterialMappingItem > materialMapping;

    /* Go though all the material elements and map them. */
    if ( const uint32_t mc = (uint32_t) pMeshFb->GetElementMaterialCount( ) ) {
        s.console->info( "Mesh \"{}\" has {} material elements.", pMeshFb->GetNode( )->GetName( ), mc );

        for ( uint32_t m = 0; m < mc; ++m ) {
            if ( const auto materialElement = pMeshFb->GetElementMaterial( m ) ) {
                switch ( const auto mappingMode = materialElement->GetMappingMode( ) ) {
                    /* Case when there is a single entry in the material element arrays. */
                    case FbxLayerElement::eAllSame: {
                        switch ( const auto referenceMode = materialElement->GetReferenceMode( ) ) {
                            case FbxLayerElement::EReferenceMode::eDirect: {
                                const auto directArray = materialElement->mDirectArray;
                                /* There must be at least 1 material. */
                                if ( !directArray || directArray->GetCount( ) < 1 ) {
                                    s.console->error( "Material element {} has no indices, skipped.", m );
                                    DebugBreak( );
                                    break;
                                }

                                for ( auto k = 0; k < pMeshFb->GetNode( )->GetMaterialCount( ); ++k ) {
                                    if ( pMeshFb->GetNode( )->GetMaterial( k ) == directArray->GetAt( 0 ) ) {
                                        /* Since the mapping mode is eAllSame, return here. */
                                        subsetsFb.emplace_back( k, 0, pMeshFb->GetPolygonCount( ) * 3 );
                                        return true;
                                    }
                                }

                                /* All went wrong, try to get corrent mappings in the next material element. */
                                s.console->error( "Failed to find mapping material in material element {}.", m );
                            } break;

                            case FbxLayerElement::EReferenceMode::eIndex:
                            case FbxLayerElement::EReferenceMode::eIndexToDirect: {
                                const auto indexArray = materialElement->mIndexArray;
                                /* There must be at least 1 material index. */
                                if ( !indexArray || indexArray->GetCount( ) < 1 ) {
                                    s.console->error( "Material element {} has no indices, skipped.", m );
                                    DebugBreak( );
                                    break;
                                }

                                /* Since the mapping mode is eAllSame, return here. */
                                subsetsFb.emplace_back( indexArray->GetAt( 0 ), 0, pMeshFb->GetPolygonCount( ) * 3 );
                                return true;
                            } break;

                            default:
                                s.console->error( "Material element #{} has unsupported {} reference mode.", m, referenceMode );
                                break;
                        }
                    } break;

                    case FbxLayerElement::eByPolygon: {
                        switch ( const auto referenceMode = materialElement->GetReferenceMode( ) ) {
                            case FbxLayerElement::EReferenceMode::eDirect: {
                                const auto directArray = materialElement->mDirectArray;
                                if ( directArray && directArray->GetCount( ) < 1 ) {
                                    s.console->error( "Material element {} has no indices, skipped.", m );
                                    DebugBreak( );
                                    break;
                                }

                                std::map< const FbxSurfaceMaterial*, uint32_t > mappingDirectToIndex;
                                for ( auto k = 0; k < pMeshFb->GetNode( )->GetMaterialCount( ); ++k ) {
                                    mappingDirectToIndex[ pMeshFb->GetNode( )->GetMaterial( k ) ] = uint32_t( k );
                                }

                                materialMapping.reserve( pMeshFb->GetPolygonCount( ) );
                                for ( uint32_t i = 0; i < (uint32_t) pMeshFb->GetPolygonCount( ); ++i ) {
                                    MaterialMappingItem materialMappingItem;
                                    materialMappingItem.materialIndex = mappingDirectToIndex[ directArray->GetAt( i ) ];
                                    materialMappingItem.polygonIndex  = uint32_t( i );
                                    materialMapping.push_back( materialMappingItem );

                                    materialIndicesInUse.insert( materialMappingItem.materialIndex );
                                }
                            } break;

                            case FbxLayerElement::EReferenceMode::eIndex:
                            case FbxLayerElement::EReferenceMode::eIndexToDirect: {

                                const auto indexArray = materialElement->mIndexArray;
                                if ( indexArray && indexArray->GetCount( ) < 2 ) {
                                    s.console->error( "Material element {} has no indices, skipped.", m );
                                    break;
                                }

                                materialMapping.reserve( pMeshFb->GetPolygonCount( ) );
                                for ( uint32_t i = 0; i < (uint32_t) pMeshFb->GetPolygonCount( ); ++i ) {
                                    MaterialMappingItem materialMappingItem;
                                    materialMappingItem.materialIndex = uint32_t( indexArray->GetAt( i ) );
                                    materialMappingItem.polygonIndex  = uint32_t( i );
                                    materialMapping.push_back( materialMappingItem );

                                    materialIndicesInUse.insert( materialMappingItem.materialIndex );
                                }
                            } break;

                            default:
                                s.console->error( "Material element #{} has unsupported {} reference mode.", m, referenceMode );
                                break;
                        }
                    } break;

                    default:
                        s.console->error( "Material element #{} has {} unsupported mapping mode.", m, mappingMode );
                        break;
                }
            }
        }
    }

    if ( materialMapping.empty( ) ) {
        s.console->error( "Mesh \"{}\" has no correctly mapped materials (fallback to first one).", pMeshFb->GetNode( )->GetName( ) );
        // Splitted meshes per material case, do not issue a debug break.
        return false;
    }

    if ( materialIndicesInUse.size( ) != pMeshFb->GetNode( )->GetMaterialCount( ) ) {
        s.console->warn( "Mesh \"{}\" has {} materials assigned, but only {} materials are mapped.",
                          pMeshFb->GetNode( )->GetName( ),
                          pMeshFb->GetNode( )->GetMaterialCount( ),
                          materialIndicesInUse.size( ) );
    }

    //
    // The most important part:
    // 1) Make sure our mapping is sorted, the FBX SDK doc does not state their data is sorted
    //    but in practice I did not see the unsorted polygon indices.
    // 2) Fill all the polygon ranges (consider breaks in the sorted ranges).
    //    If the polygon indices are [2, 3, 4, 10, 11, 12, 13, 15, 17]
    //    we will get {2, 3} (range starts at 2 and is 3 polygons long),
    //                {10, 4}, {15, 1}, {17, 1}.
    //

    // Sort materialMapping by material index, than by polygon index.
    std::sort( materialMapping.begin( ), materialMapping.end( ), [&]( const MaterialMappingItem& a, const MaterialMappingItem& b ) {
        return a.materialIndex != b.materialIndex ? a.materialIndex < b.materialIndex : a.polygonIndex < b.polygonIndex;
    } );

    // Transform mappings to ranges
    //
    // Mapping: {              ...               }
    //          { material: 1, polygonIndex: 100 }
    //          { material: 1, polygonIndex: 101 }
    //          { material: 1, polygonIndex: 102 }
    //          { material: 2, polygonIndex: 103 }
    //          { material: 2, polygonIndex: 104 }
    //          { material: 3, polygonIndex: 105 }
    //          {              ...               }
    //
    // Ranges:  {                          ...                          }
    //          { material: 1, polygonIndex: 100, polygonLastIndex: 102 }
    //          { material: 2, polygonIndex: 103, polygonLastIndex: 104 }
    //          {                          ...                          }
    //

    struct MaterialPolygonRange {
        uint32_t materialIndex;
        uint32_t polygonIndex;
        uint32_t polygonLastIndex;
    };

    std::vector< MaterialPolygonRange > polygonRanges;
    {
        uint32_t currMaterialIndex = materialMapping[ 0 ].materialIndex;
        uint32_t currPolygonIndex  = materialMapping[ 0 ].polygonIndex;

        for ( uint32_t i = 1; i < materialMapping.size( ); ++i ) {
            uint32_t materialIndex = materialMapping[ i ].materialIndex;

            if ( currMaterialIndex != materialIndex ) {
                // Case: Another material range has started, push a new subset.
                // For example: {              ...               }
                //              { material: 1, polygonIndex: 100 }
                //              { material: 1, polygonIndex: 101 }
                //              { material: 1, polygonIndex: 102 }
                //              { material: 2, polygonIndex: 103 } <== Start another subset here
                //              { material: 2, polygonIndex: 104 }
                //              {              ...               }

                MaterialPolygonRange materialPolygonRange;

                materialPolygonRange.materialIndex    = currMaterialIndex;
                materialPolygonRange.polygonIndex     = currPolygonIndex;
                materialPolygonRange.polygonLastIndex = materialMapping[ i - 1 ].polygonIndex;

                polygonRanges.push_back( materialPolygonRange );
                currMaterialIndex = materialIndex;
                currPolygonIndex  = materialMapping[ i ].polygonIndex;

            } else if ( ( materialMapping[ i ].polygonIndex - materialMapping[ i - 1 ].polygonIndex ) > 1 ) {
                // Case: There is a gap in material polygon indices
                // For example: {              ...               }
                //              { material: 2, polygonIndex: 100 }
                //              { material: 2, polygonIndex: 101 }
                //              { material: 2, polygonIndex: 102 }
                //              { material: 2, polygonIndex: 200 } <== Start another subset here
                //              { material: 2, polygonIndex: 201 }
                //              {              ...               }

                MaterialPolygonRange materialPolygonRange;

                materialPolygonRange.materialIndex    = currMaterialIndex;
                materialPolygonRange.polygonIndex     = currPolygonIndex;
                materialPolygonRange.polygonLastIndex = materialMapping[ i - 1 ].polygonIndex;

                polygonRanges.push_back( materialPolygonRange );
                currMaterialIndex = materialIndex;
                currPolygonIndex  = materialMapping[ i ].polygonIndex;

            } else if ( i == materialMapping.size( ) - 1 ) {
                // Case: The end of the mapping item collection is reached, push the last subset
                // For example: {              ...               }
                //              { material: 2, polygonIndex: 100 }
                //              { material: 2, polygonIndex: 101 }
                //              { material: 2, polygonIndex: 102 }
                //              { material: 2, polygonIndex: 103 }
                //              {              end               } <== Push the last subset here

                MaterialPolygonRange materialPolygonRange;

                materialPolygonRange.materialIndex    = currMaterialIndex;
                materialPolygonRange.polygonIndex     = currPolygonIndex;
                materialPolygonRange.polygonLastIndex = materialMapping[ i ].polygonIndex;

                polygonRanges.push_back( materialPolygonRange );
            }
        }
    }

    subsetsFb.reserve( polygonRanges.size( ) );

    // Sort polygonRanges
    std::sort( polygonRanges.begin( ), polygonRanges.end( ), [&]( const MaterialPolygonRange a, const MaterialPolygonRange b ) {

#define APEMODE_GET_SUBSETS_SORT_BY_MATERIAL_INDEX
#ifdef APEMODE_GET_SUBSETS_SORT_BY_MATERIAL_INDEX
        // Sort by material index (then by polygon index).
        return a.materialIndex != b.materialIndex ? a.materialIndex < b.materialIndex : a.polygonIndex < b.polygonIndex;
#else
        // Sort by polygon index.
        return a.polygonIndex < b.polygonIndex;
#endif
    } );

    // Transform polygon ranges to index subsets
    //
    // Ranges:  {                          ...                          }
    //          { material: 1, polygonIndex: 100, polygonLastIndex: 102 }
    //          { material: 2, polygonIndex: 103, polygonLastIndex: 104 }
    //          {                          ...                          }
    //
    // Subsets: {                ...                }
    //          { material: 1, index: 300, count: 9 }
    //          { material: 2, index: 309, count: 6 }
    //          {                ...                }
    //

    // Fill subsets
    for ( const MaterialPolygonRange range : polygonRanges ) {

        const uint32_t polygonCount = range.polygonLastIndex - range.polygonIndex + 1;
        const apemodefb::SubsetFb subsetFb( range.materialIndex, range.polygonIndex * 3, polygonCount * 3 );
        subsetsFb.push_back( subsetFb );

        s.console->info( "\t+ subset: material #{} -> base index {}, last index {} (index count {})",
                         subsetFb.material_id( ),
                         subsetFb.base_index( ),
                         subsetFb.base_index( ) + subsetFb.index_count( ) - 1,
                         subsetFb.index_count( ) );
    }

    return true;
}

//
// Helper function to get values from geometry element layers
// with their reference and mapping modes.
//

/**
 * Returns the value from the element layer by index with respect to reference mode.
 **/
template < typename TElementLayer, typename TElementValue >
TElementValue GetElementValue( const TElementLayer* pElementLayer, uint32_t i ) {
    assert( pElementLayer );
    switch ( const auto referenceMode = pElementLayer->GetReferenceMode( ) ) {
        case FbxLayerElement::EReferenceMode::eDirect:
            return pElementLayer->GetDirectArray( ).GetAt( i );
        case FbxLayerElement::EReferenceMode::eIndex:
        case FbxLayerElement::EReferenceMode::eIndexToDirect: {
            const int j = pElementLayer->GetIndexArray( ).GetAt( i );
            return pElementLayer->GetDirectArray( ).GetAt( j );
        }

        default:
            apemode::State::Get( ).console->error(
                "Reference mode {} of layer \"{}\" "
                "is not supported.",
                referenceMode,
                pElementLayer->GetName( ) );

            DebugBreak( );
            return TElementValue( );
    }
}

/**
 * Returns the value from the element layer with respect to reference and mapping modes.
 **/
template < typename TElementLayer, typename TElementValue >
TElementValue GetElementValue( const TElementLayer* pElementLayer,
                               uint32_t             controlPointIndex,
                               uint32_t             vertexIndex,
                               uint32_t             polygonIndex ) {
    if ( nullptr == pElementLayer )
        return TElementValue( );

    switch ( const auto mappingMode = pElementLayer->GetMappingMode( ) ) {
        case FbxLayerElement::EMappingMode::eByControlPoint:
            return GetElementValue< TElementLayer, TElementValue >( pElementLayer, controlPointIndex );
        case FbxLayerElement::EMappingMode::eByPolygon:
            return GetElementValue< TElementLayer, TElementValue >( pElementLayer, polygonIndex );
        case FbxLayerElement::EMappingMode::eByPolygonVertex:
            return GetElementValue< TElementLayer, TElementValue >( pElementLayer, vertexIndex );

        default:
            apemode::State::Get( ).console->error(
                "Mapping mode {} of layer \"{}\" "
                "is not supported.",
                mappingMode,
                pElementLayer->GetName( ) );

            DebugBreak( );
            
            if constexpr (std::is_same<TElementValue, FbxColor>::value) {
                return FbxColor(1.0, 1.0, 1.0, 1.0);
            }
            
            return TElementValue( );
    }
}

// https://forums.autodesk.com/t5/fbx-forum/useful-things-you-might-want-to-know-about-fbxsdk/td-p/4821177
FbxAMatrix GetGeometricTransformation( FbxNode* pFbxNode ) {
    assert( pFbxNode );
    if ( pFbxNode ) {
        const FbxVector4 lT = pFbxNode->GetGeometricTranslation( FbxNode::eSourcePivot );
        const FbxVector4 lR = pFbxNode->GetGeometricRotation( FbxNode::eSourcePivot );
        const FbxVector4 lS = pFbxNode->GetGeometricScaling( FbxNode::eSourcePivot );

        return FbxAMatrix( lT, lR, lS );
    }

    return FbxAMatrix();
}

/**
 * Returns nullptr in case element layer has unsupported properties or is null.
 **/
template < typename TElementLayer >
const TElementLayer* VerifyElementLayer( const TElementLayer* pElementLayer ) {
    if ( nullptr == pElementLayer ) {
        apemode::State::Get( ).console->warn( "Missing element layer." );
        return nullptr;
    }

    switch ( const auto mappingMode = pElementLayer->GetMappingMode( ) ) {
        case FbxLayerElement::EMappingMode::eByControlPoint:
        case FbxLayerElement::EMappingMode::eByPolygon:
        case FbxLayerElement::EMappingMode::eByPolygonVertex:
            break;
        default:
            apemode::State::Get( ).console->error(
                "Mapping mode {} of layer \"{}\" "
                "is not supported.",
                mappingMode,
                pElementLayer->GetName( ) );
            return nullptr;
    }

    switch ( const auto referenceMode = pElementLayer->GetReferenceMode( ) ) {
        case FbxLayerElement::EReferenceMode::eDirect:
        case FbxLayerElement::EReferenceMode::eIndex:
        case FbxLayerElement::EReferenceMode::eIndexToDirect:
            break;
        default:
            apemode::State::Get( ).console->error(
                "Reference mode {} of layer \"{}\" "
                "is not supported.",
                referenceMode,
                pElementLayer->GetName( ) );
            return nullptr;
    }

    return pElementLayer;
}

/**
 * Helper structure to assign vertex property values
 **/
struct StaticVertexQTangent {
    mathfu::vec3 position;
    mathfu::vec4 qtangent;
    mathfu::vec2 texCoords;
};

/**
 * Helper structure to assign vertex property values
 **/
//struct StaticSkinnedVertex {
//    mathfu::vec3 position;
//    mathfu::vec3 normal;
//    mathfu::vec4 tangent;
//    mathfu::vec2 texCoords;
//    mathfu::vec4 weights;
//    mathfu::vec4 indices;
//};

/**
 * Helper structure to assign vertex property values
 **/
//struct StaticSkinned8Vertex {
//    mathfu::vec3 position;
//    mathfu::vec3 normal;
//    mathfu::vec4 tangent;
//    mathfu::vec2 texCoords;
//    mathfu::vec4 weights_0;
//    mathfu::vec4 weights_1;
//    mathfu::vec4 indices_0;
//    mathfu::vec4 indices_1;
//};

static const uint32_t sInvalidIndex = uint32_t( -1 );

enum EBoneCountPerControlPoint : uint32_t {
    eBoneCountPerControlPoint_4 = 4,
    eBoneCountPerControlPoint_8 = 8,
    eMaxBoneCountPerControlPoint = eBoneCountPerControlPoint_8,
};

struct BoneWeightIndex {
    float    weight;
    uint32_t index;
};

template < uint32_t TBoneCountPerControlPoint = eMaxBoneCountPerControlPoint >
struct TControlPointSkinInfo {

    static const uint32_t kBoneCountPerControlPoint = TBoneCountPerControlPoint;

    std::array< BoneWeightIndex, kBoneCountPerControlPoint > weights;

    TControlPointSkinInfo( ) {
        for ( uint32_t i = 0; i < kBoneCountPerControlPoint; ++i ) {
            weights[ i ] = {0.0f, sInvalidIndex};
        }
    }

    void OnBoneAdded( ) {
        std::sort( weights.begin( ), weights.end( ), [&]( const BoneWeightIndex& a, const BoneWeightIndex& b ) {
            return a.weight > b.weight;
        } );
    }

    /** Assignes bone to the control point.
     * @param weight Bone influence weight.
     * @param index Bone index.
     * @return true of the bone was added, false otherwise.
     **/
    bool AddBone( float weight, uint32_t index ) {
        if (weight < std::numeric_limits<float>::epsilon()) {
            return false;
        }
    
        for ( uint32_t i = 0; i < kBoneCountPerControlPoint; ++i ) {
            if ( weights[ i ].index == index ) {
                weights[ i ].weight += weight;
                assert( false );
                return true;
            }
        }

        for ( uint32_t i = 0; i < kBoneCountPerControlPoint; ++i ) {
            if ( weights[ i ].index == sInvalidIndex ) {
                weights[ i ] = {weight, index};
                OnBoneAdded( );
                return true;
            }
        }

        uint32_t minIndex = 0;
        float minWeight = weights[ 0 ].weight;

        for ( uint32_t i = 1; i < kBoneCountPerControlPoint; ++i ) {
            if ( weights[ i ].weight < minWeight ) {
                minWeight = weights[ i ].weight;
                minIndex  = i;
            }
        }

        if ( minWeight < weight ) {
            weights[ minIndex ] = {weight, index};
            OnBoneAdded( );
            DebugBreak( );
            return true;
        }

        return false;
    }

    uint32_t GetUsedSlotCount() {
        uint32_t usedSlotCount = 0;
        for ( uint32_t i = 0; i < kBoneCountPerControlPoint; ++i ) {
            usedSlotCount += ( weights[ i ].index != sInvalidIndex );
        }
        return usedSlotCount;
    }

    /** Prepares assigned bones for GPU skinning.
     * @note Ensures the sum fo the weights is exactly one.
     *       Ensures the indices are valid (vertex packing will get zero or positive bone index).
     **/
    void NormalizeWeights( uint32_t bonesPerVertex, uint32_t defaultIndex ) {
        for ( uint32_t i = 0; i < kBoneCountPerControlPoint; ++i ) {
            if ( weights[ i ].index == sInvalidIndex ) {
                assert( 0.0f == weights[ i ].weight );
                weights[ i ].index = defaultIndex;
            }
        }

        float totalWeight = float( 0 );

        for ( uint32_t i = 0; i < bonesPerVertex; ++i ) {
            totalWeight += weights[ i ].weight;
        }

        for ( uint32_t i = 0; i < bonesPerVertex; ++i ) {
            weights[ i ].weight /= totalWeight;
        }
    }

    template < EBoneCountPerControlPoint TBoneCount >
    std::array< float, TBoneCount > Compile( ) {
        float totalWeight = float( 0 );
        for ( uint32_t i = 0; i < TBoneCount; ++i ) {
            assert( weights[ i ].weight <= 1.0f );
            totalWeight += weights[ i ].weight;
        }

        std::array< float, TBoneCount > compiled;
        for ( uint32_t i = 0; i < TBoneCount; ++i ) {
            compiled[ i ] = weights[ i ].index;
        }

        if ( weights[ 0 ].weight >= 0.99f ) {
            float halfWeight = totalWeight * 0.5f;
            assert( halfWeight < 1.0f );
            
            compiled[ 0 ] += halfWeight;
            compiled[ 1 ] = compiled[ 0 ];
            assert( uint32_t( compiled[ 0 ] ) == weights[ 0 ].index );
            assert( uint32_t( compiled[ 1 ] ) == weights[ 0 ].index );
        } else {
            for ( uint32_t i = 0; i < TBoneCount; ++i ) {
                assert( weights[ i ].weight < 1.0f );
                compiled[ i ] += weights[ i ].weight;
                assert( uint32_t( compiled[ i ] ) == weights[ i ].index );
            }
        }
        
        return compiled;
    }
};

//
// Flatbuffers takes care about correct platform-independent alignment.
//

//static_assert( sizeof( StaticVertex ) == sizeof( apemodefb::StaticVertexFb ), "Must match" );
//static_assert( sizeof( StaticSkinnedVertex ) == sizeof( apemodefb::StaticSkinnedVertexFb ), "Must match" );
//static_assert( sizeof( StaticSkinned8Vertex ) == sizeof( apemodefb::StaticSkinned8VertexFb ), "Must match" );

bool IsValidNormal( FbxVector4 values ) {
    if ( ( isnan( values[ 0 ] ) || isinf( values[ 0 ] ) ) ||
         ( isnan( values[ 1 ] ) || isinf( values[ 1 ] ) ) ||
         ( isnan( values[ 2 ] ) || isinf( values[ 2 ] ) ) ) {
        return false;
    }

    const double length = values.Length( );
    if ( length < std::numeric_limits< float >::epsilon( ) ) {
        return false;
    }

    return true;
}

struct VertexInitializationResult {
    bool bValidTangents = false;
};

/**
 * Initialize vertices with very basic properties like 'position', 'normal', 'tangent', 'texCoords'.
 * Calculate mesh position and texcoord min max values.
 **/
VertexInitializationResult InitializeVertices( FbxMesh* mesh, apemode::Mesh& m, StaticVertex* vertices, size_t vertexCount ) {
    auto& s = apemode::State::Get( );

    VertexInitializationResult result;

    const uint32_t cc = (uint32_t) mesh->GetControlPointsCount( );
    const uint32_t pc = (uint32_t) mesh->GetPolygonCount( );

    s.console->info( "Mesh \"{}\" has {} control points.", mesh->GetNode( )->GetName( ), cc );
    s.console->info( "Mesh \"{}\" has {} polygons.", mesh->GetNode( )->GetName( ), pc );

    mathfu::vec3 positionMin;
    mathfu::vec3 positionMax;
    mathfu::vec2 texcoordMin;
    mathfu::vec2 texcoordMax;

    positionMin.x = std::numeric_limits< float >::max( );
    positionMin.y = std::numeric_limits< float >::max( );
    positionMin.z = std::numeric_limits< float >::max( );
    positionMax.x = std::numeric_limits< float >::min( );
    positionMax.y = std::numeric_limits< float >::min( );
    positionMax.z = std::numeric_limits< float >::min( );

    texcoordMin.x = std::numeric_limits< float >::max( );
    texcoordMin.y = std::numeric_limits< float >::max( );
    texcoordMax.x = std::numeric_limits< float >::min( );
    texcoordMax.y = std::numeric_limits< float >::min( );

    const auto uve = VerifyElementLayer( mesh->GetElementUV( ) );
    auto       ne  = VerifyElementLayer( mesh->GetElementNormal( ) );
    auto       te  = VerifyElementLayer( mesh->GetElementTangent( ) );
    auto       ce  = VerifyElementLayer( mesh->GetElementVertexColor( ) );

    uint32_t vi = 0;
    for ( uint32_t pi = 0; pi < pc; ++pi ) {
        assert( 3 == mesh->GetPolygonSize( pi ) );

        // Having this array we can easily control polygon winding order.
        // Since mesh is triangular we can make it static [3] at compile-time.
        // for ( const uint32_t pvi : {0, 1, 2} ) {
        for ( const uint32_t pvi : {0, 1, 2} ) {
            const uint32_t ci = (uint32_t) mesh->GetPolygonVertex( (int) pi, (int) pvi );

            const auto cp = mesh->GetControlPointAt( ci );
            const auto uv = GetElementValue< FbxGeometryElementUV, FbxVector2 >( uve, ci, vi, pi );
            FbxVector4 n  = GetElementValue< FbxGeometryElementNormal, FbxVector4 >( ne, ci, vi, pi );
            FbxVector4 t  = GetElementValue< FbxGeometryElementTangent, FbxVector4 >( te, ci, vi, pi );
            FbxColor c    = GetElementValue< FbxGeometryElementVertexColor, FbxColor >( ce, ci, vi, pi );

            n.Normalize();
            t.Normalize();

            AssertValidVec3(cp);
            AssertValidVec3(n);
            AssertValidVec3(t);
            AssertValidVec3(uv);

            if ( ne && !IsValidNormal( n ) ) {
                ne = nullptr;
                s.console->warn( "Mesh \"{}\" has invalid normals", mesh->GetNode( )->GetName( ) );
                s.console->warn( "Mesh \"{}\" will have generated normals", mesh->GetNode( )->GetName( ) );
            }
            if ( te && !IsValidNormal( t ) ) {
                te = nullptr;
                s.console->warn( "Mesh \"{}\" has invalid tangents", mesh->GetNode( )->GetName( ) );
                s.console->warn( "Mesh \"{}\" will have generated tangents", mesh->GetNode( )->GetName( ) );
            }
            if ( ce && !c.IsValid( ) ) {
                ce = nullptr;
                s.console->warn( "Mesh \"{}\" has invalid colors", mesh->GetNode( )->GetName( ) );
                s.console->warn( "Mesh \"{}\" will have white colors", mesh->GetNode( )->GetName( ) );
            }

            StaticVertex& vvii  = vertices[ vi ];
            vvii.position[ 0 ]  = cp[ 0 ];
            vvii.position[ 1 ]  = cp[ 1 ];
            vvii.position[ 2 ]  = cp[ 2 ];
            vvii.normal[ 0 ]    = n[ 0 ];
            vvii.normal[ 1 ]    = n[ 1 ];
            vvii.normal[ 2 ]    = n[ 2 ];
            vvii.tangent[ 0 ]   = t[ 0 ];
            vvii.tangent[ 1 ]   = t[ 1 ];
            vvii.tangent[ 2 ]   = t[ 2 ];
            vvii.tangent[ 3 ]   = t[ 3 ];
            vvii.texCoords[ 0 ] = uv[ 0 ];
            vvii.texCoords[ 1 ] = uv[ 1 ];
            vvii.color[ 0 ]   = c[ 0 ];
            vvii.color[ 1 ]   = c[ 1 ];
            vvii.color[ 2 ]   = c[ 2 ];
            vvii.color[ 3 ]   = c[ 3 ];
            vvii.controlPointIndex = ci;

            assert( !isnan( (float) cp[ 0 ] ) && !isnan( (float) cp[ 1 ] ) && !isnan( (float) cp[ 2 ] ) );
            assert( !isnan( (float) n[ 0 ] ) && !isnan( (float) n[ 1 ] ) && !isnan( (float) n[ 2 ] ) );
            assert( !isnan( (float) t[ 0 ] ) && !isnan( (float) t[ 1 ] ) && !isnan( (float) t[ 2 ] ) && !isnan( (float) t[ 3 ] ) );
            assert( !isnan( (float) uv[ 0 ] ) && !isnan( (float) uv[ 1 ] ) );

            positionMin.x = std::min( positionMin.x, (float) cp[ 0 ] );
            positionMin.y = std::min( positionMin.y, (float) cp[ 1 ] );
            positionMin.z = std::min( positionMin.z, (float) cp[ 2 ] );
            positionMax.x = std::max( positionMax.x, (float) cp[ 0 ] );
            positionMax.y = std::max( positionMax.y, (float) cp[ 1 ] );
            positionMax.z = std::max( positionMax.z, (float) cp[ 2 ] );

            texcoordMin.x = std::min( texcoordMin.x, (float) uv[ 0 ] );
            texcoordMin.y = std::min( texcoordMin.y, (float) uv[ 1 ] );
            texcoordMax.x = std::max( texcoordMax.x, (float) uv[ 0 ] );
            texcoordMax.y = std::max( texcoordMax.y, (float) uv[ 1 ] );

            ++vi;
        }
    }

    m.positionMin = apemodefb::Vec3Fb( positionMin.x, positionMin.y, positionMin.z );
    m.positionMax = apemodefb::Vec3Fb( positionMax.x, positionMax.y, positionMax.z );
    m.texcoordMin = apemodefb::Vec2Fb( texcoordMin.x, texcoordMin.y );
    m.texcoordMax = apemodefb::Vec2Fb( texcoordMax.x, texcoordMax.y );

    if ( nullptr == uve ) {
        s.console->error( "Mesh \"{}\" does not have texcoords geometry layer.",
                          mesh->GetNode( )->GetName( ) );
    }

    if ( nullptr == ne ) {
        s.console->warn( "Mesh \"{}\" does not have normal geometry layer.",
                          mesh->GetNode( )->GetName( ) );

        // Calculate face normals ourselves.
        // Usable but incorrect.
        CalculateFaceNormals( vertices, vertexCount );
    }

    result.bValidTangents = te != nullptr;
    if ( !te && uve ) {
        s.console->warn( "Mesh \"{}\" does not have tangent geometry layer.",
                         mesh->GetNode( )->GetName( ) );

        // Calculate tangents ourselves if UVs are available.
        result.bValidTangents = CalculateTangents( vertices, vertexCount );
    } else if ( !te && !uve ) {
        s.console->warn( "Mesh \"{}\" does not have tangent and texcoords geometry layers.",
                         mesh->GetNode( )->GetName( ) );

        // Calculate tangents ourselves if UVs are available.
        result.bValidTangents = CalculateTangentsNoUVs( vertices, vertexCount );
    }

    if (result.bValidTangents && te) {
        for (uint32_t i = 0; i < vertexCount; ++i) {
            StaticVertex& vvii = vertices[ i ];

            const mathfu::dvec3 nn = vvii.normal.Normalized();
            const mathfu::dvec3 tt = vvii.tangent.xyz().Normalized();
            const mathfu::dvec3 ttt = mathfu::normalize( tt - nn * mathfu::dot( nn, tt ) );
            AssertValidVec3(nn);
            AssertValidVec3(tt);
            AssertValidVec3(ttt);

            vvii.normal[ 0 ] = nn.x;
            vvii.normal[ 1 ] = nn.y;
            vvii.normal[ 2 ] = nn.z;

            vvii.tangent[ 0 ] = ttt.x;
            vvii.tangent[ 1 ] = ttt.y;
            vvii.tangent[ 2 ] = ttt.z;

            if (vvii.tangent[ 3 ] < 0.0) {
                vvii.tangent[ 3 ] = -1.0f;
            } else {
                vvii.tangent[ 3 ] = +1.0f;
            }
        }
    } else {
        for (uint32_t i = 0; i < vertexCount; ++i) {
            StaticVertex& vvii = vertices[ i ];
            const mathfu::dvec3 nn = vvii.normal.Normalized();

            vvii.normal[ 0 ] = nn.x;
            vvii.normal[ 1 ] = nn.y;
            vvii.normal[ 2 ] = nn.z;
        }
    }

    return result;
}

//
// See implementation in fbxpmeshopt.cpp.
//

std::string ToPrettySizeString( size_t size );
// void Optimize32( apemode::Mesh& mesh, apemodefb::StaticVertexFb const* vertices, uint32_t & vertexCount, uint32_t vertexStride );
// void Optimize16( apemode::Mesh& mesh, apemodefb::StaticVertexFb const* vertices, uint32_t & vertexCount, uint32_t vertexStride );

//
// See implementation in fbxpmeshpacking.cpp.
//

//void Pack( const apemodefb::StaticVertexFb* vertices,
//           apemodefb::PackedVertexFb*       packed,
//           const uint32_t                   vertexCount,
//           const mathfu::vec3               positionMin,
//           const mathfu::vec3               positionMax,
//           const mathfu::vec2               texcoordsMin,
//           const mathfu::vec2               texcoordsMax );
//
//uint32_t PackedMaxBoneCount( );
//
//void Pack( const apemodefb::StaticSkinnedVertexFb* vertices,
//           apemodefb::PackedSkinnedVertexFb*       packed,
//           const uint32_t                          vertexCount,
//           const mathfu::vec3                      positionMin,
//           const mathfu::vec3                      positionMax,
//           const mathfu::vec2                      texcoordsMin,
//           const mathfu::vec2                      texcoordsMax );

// http://jcgt.org/published/0003/02/01/paper.pdf
// Assume normalized input on +Z hemisphere.
// Output is on [-1, 1].
mathfu::dvec2 ToHemioct(mathfu::dvec3 v) {
    // Project the hemisphere onto the hemi-octahedron,
    // and then into the xy plane
    mathfu::dvec2 p = v.xy() * (1.0 / (abs(v.x) + abs(v.y) + v.z));
    // Rotate and scale the center diamond to the unit square
    return mathfu::dvec2(p.x + p.y, p.x - p.y);
}

mathfu::dvec3 FromHemioct(mathfu::dvec2 e) {
    // Rotate and scale the unit square back to the center diamond
    mathfu::dvec2 temp = mathfu::dvec2(e.x + e.y, e.x - e.y) * 0.5;
    mathfu::dvec3 v = mathfu::dvec3(temp, 1.0 - abs(temp.x) - abs(temp.y));
    return normalize(v);
}

mathfu::dquat GetQTangent(const mathfu::dvec3 normal, const mathfu::dvec3 tangent, double reflection) {
    assert(reflection != 0.0);
    
    // https://bitbucket.org/sinbad/ogre/src/18ebdbed2edc61d30927869c7fb0cf3ae5697f0a/OgreMain/src/OgreSubMesh2.cpp?at=v2-1&fileviewer=file-view-default#OgreSubMesh2.cpp-988
    const mathfu::dvec3 bitangent( mathfu::cross( normal, tangent ).Normalized( ) );
    AssertValidVec3( bitangent );
    
    mathfu::dmat3 tangentFrame( normal.x, normal.y, normal.z,
                                tangent.x, tangent.y, tangent.z,
                                bitangent.x, bitangent.y, bitangent.z );

    mathfu::dquat q = mathfu::dquat::FromMatrix( tangentFrame );
    q.Normalize();

    if ( q.scalar( ) < 0.0 ) {
        q = mathfu::dquat( -q.scalar( ), -q.vector( ) );
    }

    // TODO: Should be in use for packing it into ints
    //       because the "-" sign will be lost.
    const double bias = 1.0 / 32767.0; // ( pow( 2.0, 15.0 ) - 1.0 );
    if ( q.scalar( ) < bias ) {
        auto normFactor = sqrt( 1.0 - bias * bias );
        q = mathfu::dquat( bias, q.vector() * normFactor );
    }

    if ( q.scalar( ) < 0.0 ) {
        q = mathfu::dquat( -q.scalar( ), -q.vector( ) );
    }

    if ( reflection < 0.0 ) {
        q = mathfu::dquat( -q.scalar( ), -q.vector( ) );
    }

    AssertValidFloat( q.scalar( ) );
    AssertValidVec3( q.vector( ) );
    
    assert(mathfu::dot(tangentFrame.GetColumn(0), tangentFrame.GetColumn(1)) < std::numeric_limits< float >::epsilon());
    assert(mathfu::dot(tangentFrame.GetColumn(1), tangentFrame.GetColumn(2)) < std::numeric_limits< float >::epsilon());
    assert(mathfu::dot(tangentFrame.GetColumn(0), tangentFrame.GetColumn(2)) < std::numeric_limits< float >::epsilon());

    return q;
}

enum class EVertexOrder { CW, CCW };

template < typename TIndex, EVertexOrder TOrder = EVertexOrder::CCW >
void ExportMesh( FbxNode*       pNode,
                 FbxMesh*       pMesh,
                 apemode::Node& n,
                 apemode::Mesh& m,
                 uint32_t       vertexCount,
                 bool           pack,
                 FbxSkin*       pSkin,
                 bool           optimize ) {
    /* Packing is disabled for now. */
    pack = false;

    auto& s = apemode::State::Get( );

    /* Fill indices. */

    m.indices.resize( vertexCount * sizeof( TIndex ) );
    TIndex* indices = (TIndex*) m.indices.data( );

    if ( TOrder == EVertexOrder::CW ) {
        for ( TIndex i = 0; i < vertexCount; i += 3 ) {
            indices[ i + 0 ] = i + 0;
            indices[ i + 1 ] = i + 2;
            indices[ i + 2 ] = i + 1;
        }
    }
    else {
        for ( TIndex i = 0; i < vertexCount; i += 3 ) {
            indices[ i + 0 ] = i + 0;
            indices[ i + 1 ] = i + 1;
            indices[ i + 2 ] = i + 2;
        }
    }

    static_assert(std::is_same< TIndex, uint16_t >::value ||
                  std::is_same< TIndex, uint32_t >::value,
                  "Should be either uint16 or uint32");

    // TODO: constexpr
    if ( std::is_same< TIndex, uint16_t >::value ) {
        m.indexType = apemodefb::EIndexTypeFb_UInt16;
    } else {
        const bool isUint32 = std::is_same< TIndex, uint32_t >::value;
        assert( isUint32 && "Caught unexpected index type." );
        m.indexType = apemodefb::EIndexTypeFb_UInt32;
        (void)isUint32;
    }

    /* Fill subsets. */

    GetSubsets( pMesh, m.subsets );
    if ( m.subsets.empty( ) ) {
        /* Independently from GetSubsets implementation make sure there is at least one subset. */
        const uint32_t materialId = pMesh->GetNode( )->GetMaterialCount( ) > 0 ? 0 : uint32_t( -1 );
        m.subsets.push_back( apemodefb::SubsetFb( materialId, 0, vertexCount ) );
    }

    std::vector< StaticVertex > vertices;
    vertices.resize( vertexCount );
    auto initResult = InitializeVertices( pMesh, m, vertices.data( ), vertexCount );
    (void)initResult;

    std::vector< mathfu::dquat > qtangents;
    qtangents.resize( vertexCount );
    for ( uint32_t i = 0; i < vertexCount; ++i ) {
        mathfu::dvec3 n( vertices[ i ].normal );
        mathfu::dvec3 t( vertices[ i ].tangent.xyz( ) );
        
        if ( n.Length( ) < std::numeric_limits< float >::epsilon( ) ||
             t.Length( ) < std::numeric_limits< float >::epsilon( ) ) {
            continue;
        }
        
        n.Normalize( );
        t.Normalize( );
    
        qtangents[ i ] = GetQTangent( n, t, vertices[ i ].tangent.w );
    }

    using ControlPointSkinInfo = TControlPointSkinInfo<>;

    EBoneCountPerControlPoint           boneCount = EBoneCountPerControlPoint( 0 );
    std::vector< ControlPointSkinInfo > skinInfos;

    if ( pSkin ) {
        /* Allocate skin info for each control point. */
        /* Populate skin info for each control point. */

        skinInfos.resize( pMesh->GetControlPointsCount( ) );

        m.skinId = uint32_t( s.skins.size( ) );

        s.skins.emplace_back( );
        auto& skin  = s.skins.back( );
        skin.nameId = s.PushValue( pSkin->GetName( ) );

        const int clusterCount = pSkin->GetClusterCount( );
        s.console->info( "\t Skin has {} clusters", clusterCount );

        skin.linkIds.reserve( clusterCount );
        for ( int i = 0; i < clusterCount; ++i ) {
            assert( i < clusterCount );
            const auto pCluster = pSkin->GetCluster( i );
            assert( pCluster );

            if ( const auto indexCount = pCluster->GetControlPointIndicesCount( ) ) {
                assert( s.nodeDict.find( pCluster->GetLink( )->GetUniqueID( ) ) != s.nodeDict.end( ) );
                const uint32_t linkNodeId = s.nodeDict[ pCluster->GetLink( )->GetUniqueID( ) ];

                const double* pWeights = pCluster->GetControlPointWeights( );
                const int* pIndices = pCluster->GetControlPointIndices( );

                s.console->info( "\t Cluster #{} influences {} point(s)", i, indexCount );

                uint32_t boneIndex = (uint32_t) skin.linkIds.size( );
                skin.linkIds.push_back( linkNodeId );

                for ( int j = 0; j < indexCount; ++j ) {
                    /* Assign bone (weight + index) for the control point. */
                    assert( pIndices[ j ] < pMesh->GetControlPointsCount( ) );
                    skinInfos[ pIndices[ j ] ].AddBone( (float) pWeights[ j ], boneIndex );
                }

                FbxAMatrix bindPoseMatrix;
                FbxAMatrix transformMatrix;
                pCluster->GetTransformLinkMatrix( bindPoseMatrix );
                pCluster->GetTransformMatrix( transformMatrix );
                const FbxAMatrix geometricMatrix = GetGeometricTransformation( pMesh->GetNode( ) );
                const FbxAMatrix invBindPoseMatrix = bindPoseMatrix.Inverse( ) * transformMatrix * geometricMatrix;
                skin.invBindPoseMatrices.push_back( apemode::Cast( invBindPoseMatrix ) );

                // TODO: Scaling can't be included into dual quaternions.
                // const FbxVector4 S = invBindPoseMatrix.GetS();

                const FbxQuaternion Q = invBindPoseMatrix.GetQ( );
                const FbxVector4 T = invBindPoseMatrix.GetT( );

                const FbxDualQuaternion DQ( Q, T );
                skin.invBindPoseDualQuats.push_back( apemode::Cast( DQ ) );
            }
        }

        /* Report about used bone slots. */
        uint32_t maxBoneCount = 4;
        boneCount = eBoneCountPerControlPoint_4;
        {
            std::map< uint32_t, uint32_t > boneCountToControlPointCountMap;
            for ( uint32_t i = 0; i < skinInfos.size( ); ++i ) {
                ++boneCountToControlPointCountMap[ skinInfos[ i ].GetUsedSlotCount( ) ];
            }

            s.console->info( "Used slots:" );
            for ( auto& p : boneCountToControlPointCountMap ) {
                s.console->info( "\t {} slots <- {} points ", p.first, p.second );
            }

            maxBoneCount = std::max( boneCountToControlPointCountMap.rbegin( )->first, maxBoneCount );
            s.console->info( "\t max bones <- {}", maxBoneCount );
        }


        if ( maxBoneCount > eBoneCountPerControlPoint_4 ) {
            boneCount = eBoneCountPerControlPoint_8;
        }

        /* Normalize bone weights for each control point. */
        for ( auto& skinInfo : skinInfos ) {
            skinInfo.NormalizeWeights( maxBoneCount, 0 );
        }
    }
    
    apemodefb::EVertexFormatFb eVertexFmt = apemodefb::EVertexFormatFb(-1);
    
    apemodefb::ECompressionTypeFb eCompressionType = apemodefb::ECompressionTypeFb_None;
    const std::string& meshCompression = s.options[ "mesh-compression" ].as< std::string >( );
    if ( m.subsets.size( ) == 1 && s.options[ "mesh-compression" ].count() && meshCompression != "none" ) {
        size_t stride = 0;
        size_t strideUnskinned = 0;
        
        strideUnskinned = sizeof( apemodefb::DecompressedVertexFb );
        if ( skinInfos.empty( ) ) {
            stride = sizeof( apemodefb::DecompressedVertexFb );
            eVertexFmt = apemodefb::EVertexFormatFb_Decompressed;
        } else {
            assert( boneCount == eBoneCountPerControlPoint_4 ||
                    boneCount == eBoneCountPerControlPoint_8 );

            switch ( boneCount ) {
            case eBoneCountPerControlPoint_4:
                stride = sizeof( apemodefb::DecompressedSkinnedVertexFb );
                eVertexFmt = apemodefb::EVertexFormatFb_DecompressedSkinned;
                break;
            case eBoneCountPerControlPoint_8:
                stride = sizeof( apemodefb::DecompressedFatSkinnedVertexFb );
                eVertexFmt = apemodefb::EVertexFormatFb_DecompressedFatSkinned;
                break;
            }
        }
        
        assert( stride != 0 );
        m.vertices.resize( stride * vertexCount );

        for ( uint32_t i = 0; i < vertexCount; ++i ) {
            union {
                struct {
                    uint8_t reflection : 4;
                    uint8_t index : 4;
                };

                uint8_t reflection_index;
            } p;


            assert( ( stride * i ) < m.vertices.size( ) );
            auto& dst = *reinterpret_cast< apemodefb::DecompressedVertexFb* >( m.vertices.data( ) + stride * i );

            dst.mutable_position( ).mutate_x( vertices[ i ].position.x );
            dst.mutable_position( ).mutate_y( vertices[ i ].position.y );
            dst.mutable_position( ).mutate_z( vertices[ i ].position.z );
            dst.mutable_uv( ).mutate_x( vertices[ i ].texCoords.x );
            dst.mutable_uv( ).mutate_y( vertices[ i ].texCoords.y );

            dst.mutable_normal( ).mutate_x( vertices[ i ].normal.x );
            dst.mutable_normal( ).mutate_y( vertices[ i ].normal.y );
            dst.mutable_normal( ).mutate_z( vertices[ i ].normal.z );
            dst.mutable_tangent( ).mutate_x( vertices[ i ].tangent.x );
            dst.mutable_tangent( ).mutate_y( vertices[ i ].tangent.y );
            dst.mutable_tangent( ).mutate_z( vertices[ i ].tangent.z );
            
//            auto normalHemioct  = ToHemioct( vertices[ i ].normal );
//            auto tangentHemioct = ToHemioct( vertices[ i ].tangent.xyz( ) );
//            dst.mutable_normal_hemioct( ).mutate_x( normalHemioct.x );
//            dst.mutable_normal_hemioct( ).mutate_y( normalHemioct.y );
//            dst.mutable_tangent_hemioct( ).mutate_x( tangentHemioct.x );
//            dst.mutable_tangent_hemioct( ).mutate_y( tangentHemioct.y );

            dst.mutable_color( ).mutate_x( vertices[ i ].color.x );
            dst.mutable_color( ).mutate_y( vertices[ i ].color.y );
            dst.mutable_color( ).mutate_z( vertices[ i ].color.z );
            dst.mutable_color( ).mutate_w( vertices[ i ].color.w );

            p.reflection = uint8_t( vertices[i].tangent.w >= 0 ? 1 : 0 );
            p.index = uint8_t( i % 3 );
            dst.mutate_reflection_index_packed( p.reflection_index );
        }

        if ( !skinInfos.empty( ) ) {
            union {
                struct {
                    uint8_t _0;
                    uint8_t _1;
                    uint8_t _2;
                    uint8_t _3;
                };

                uint32_t _0123;
            } joint_index_packer;

            // auto & skin = s.skins[ m.skinId ];
            for ( uint32_t i = 0; i < vertexCount; ++i ) {
                auto controlPointIndex = vertices[ i ].controlPointIndex;
                auto& skinInfo = skinInfos[ controlPointIndex ];
                
                assert( boneCount == eBoneCountPerControlPoint_4 ||
                        boneCount == eBoneCountPerControlPoint_8 );

                switch ( boneCount ) {
                    case eBoneCountPerControlPoint_4: {
                        assert( skinInfo.weights[ 0 ].index < 255 );
                        assert( skinInfo.weights[ 1 ].index < 255 );
                        assert( skinInfo.weights[ 2 ].index < 255 );
                        assert( skinInfo.weights[ 3 ].index < 255 );
                        
                        joint_index_packer._0 = skinInfo.weights[ 0 ].index;
                        joint_index_packer._1 = skinInfo.weights[ 1 ].index;
                        joint_index_packer._2 = skinInfo.weights[ 2 ].index;
                        joint_index_packer._3 = skinInfo.weights[ 3 ].index;

                        assert( ( stride * i ) < m.vertices.size( ) );
                        auto dst = reinterpret_cast< apemodefb::DecompressedSkinnedVertexFb* >( m.vertices.data( ) + stride * i );
                        dst->mutate_joint_indices( joint_index_packer._0123 );
                        dst->mutable_joint_weights( ).mutate_x( skinInfo.weights[ 0 ].weight );
                        dst->mutable_joint_weights( ).mutate_y( skinInfo.weights[ 1 ].weight );
                        dst->mutable_joint_weights( ).mutate_z( skinInfo.weights[ 2 ].weight );
                        dst->mutable_joint_weights( ).mutate_w( skinInfo.weights[ 4 ].weight );
                        
                    } break;
                    case eBoneCountPerControlPoint_8: {
                        assert( ( stride * i ) < m.vertices.size( ) );
                        auto dst = reinterpret_cast< apemodefb::DecompressedFatSkinnedVertexFb* >( m.vertices.data( ) + stride * i );

                        assert( skinInfo.weights[ 0 ].index < 255 );
                        assert( skinInfo.weights[ 1 ].index < 255 );
                        assert( skinInfo.weights[ 2 ].index < 255 );
                        assert( skinInfo.weights[ 3 ].index < 255 );
                        assert( skinInfo.weights[ 4 ].index < 255 );
                        assert( skinInfo.weights[ 5 ].index < 255 );
                        assert( skinInfo.weights[ 6 ].index < 255 );
                        assert( skinInfo.weights[ 7 ].index < 255 );

                        joint_index_packer._0 = skinInfo.weights[ 0 ].index;
                        joint_index_packer._1 = skinInfo.weights[ 1 ].index;
                        joint_index_packer._2 = skinInfo.weights[ 2 ].index;
                        joint_index_packer._3 = skinInfo.weights[ 3 ].index;
                        dst->mutable_decompressed_skinned( ).mutate_joint_indices( joint_index_packer._0123 );
                        dst->mutable_decompressed_skinned( ).mutable_joint_weights( ).mutate_x( skinInfo.weights[ 0 ].weight );
                        dst->mutable_decompressed_skinned( ).mutable_joint_weights( ).mutate_y( skinInfo.weights[ 1 ].weight );
                        dst->mutable_decompressed_skinned( ).mutable_joint_weights( ).mutate_z( skinInfo.weights[ 2 ].weight );
                        dst->mutable_decompressed_skinned( ).mutable_joint_weights( ).mutate_w( skinInfo.weights[ 3 ].weight );

                        joint_index_packer._0 = skinInfo.weights[ 4 ].index;
                        joint_index_packer._1 = skinInfo.weights[ 5 ].index;
                        joint_index_packer._2 = skinInfo.weights[ 6 ].index;
                        joint_index_packer._3 = skinInfo.weights[ 7 ].index;
                        dst->mutate_extra_joint_indices( joint_index_packer._0123 );
                        dst->mutable_extra_joint_weights( ).mutate_x( skinInfo.weights[ 4 ].weight );
                        dst->mutable_extra_joint_weights( ).mutate_y( skinInfo.weights[ 5 ].weight );
                        dst->mutable_extra_joint_weights( ).mutate_z( skinInfo.weights[ 6 ].weight );
                        dst->mutable_extra_joint_weights( ).mutate_w( skinInfo.weights[ 7 ].weight );
                        
                    } break;
                }
            }
        }
        
        draco::MeshEncoderMethod encoderMethod = draco::MESH_EDGEBREAKER_ENCODING;
        if ( meshCompression != "draco-edgebreaker" ) {
            assert( meshCompression == "draco-esequential" );
            encoderMethod = draco::MESH_SEQUENTIAL_ENCODING;
        }
        
        draco::TriangleSoupMeshBuilder builder;
        assert( vertexCount % 3 == 0 );
        builder.Start( vertexCount / 3 );
        
        const int positionAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::POSITION, 3, draco::DataType::DT_FLOAT32 );
        const int uvAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::TEX_COORD, 2, draco::DataType::DT_FLOAT32 );
        const int normalAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::NORMAL, 3, draco::DataType::DT_FLOAT32 );
        const int tangentAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::NORMAL, 3, draco::DataType::DT_FLOAT32 );
        const int colorAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::COLOR, 4, draco::DataType::DT_FLOAT32 );
        const int reflectionIndexAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::GENERIC, 1, draco::DataType::DT_UINT8 );
        int jointIndicesAttributeIndex = -1;
        int jointWeightsAttributeIndex = -1;
        int extraJointIndicesAttributeIndex = -1;
        int extraJointWeightsAttributeIndex = -1;
        
        auto jointIndicesType = draco::GeometryAttribute::Type::GENERIC;
        auto jointIndicesComponentCount = 4;
        auto jointIndicesDataType = draco::DT_UINT8;
        auto jointIndicesQuantizationBits = 8;
        
        auto jointWeightsType = draco::GeometryAttribute::Type::COLOR;
        auto jointWeightsComponentCount = 4;
        auto jointWeightsDataType = draco::DT_FLOAT32;
        auto jointWeightsQuantizationBits = 16;
        
        switch ( eVertexFmt ) {
        case apemodefb::EVertexFormatFb_Decompressed:
            break;
        case apemodefb::EVertexFormatFb_DecompressedSkinned:
            jointIndicesAttributeIndex = builder.AddAttribute( jointIndicesType, jointIndicesComponentCount, jointIndicesDataType );
            jointWeightsAttributeIndex = builder.AddAttribute( jointWeightsType, jointWeightsComponentCount, jointWeightsDataType );
            break;
        case apemodefb::EVertexFormatFb_DecompressedFatSkinned:
            jointIndicesAttributeIndex = builder.AddAttribute( jointIndicesType, jointIndicesComponentCount, jointIndicesDataType );
            jointWeightsAttributeIndex = builder.AddAttribute( jointWeightsType, jointWeightsComponentCount, jointWeightsDataType );
            extraJointIndicesAttributeIndex = builder.AddAttribute( jointIndicesType, jointIndicesComponentCount, jointIndicesDataType );
            extraJointWeightsAttributeIndex = builder.AddAttribute( jointWeightsType, jointWeightsComponentCount, jointWeightsDataType );
            break;
        default:
            assert(false);
            break;
        }
        
        for ( uint32_t i = 0; i < vertexCount; i += 3 ) {
            const draco::FaceIndex faceIndex = draco::FaceIndex( i / 3 );
            apemodefb::DecompressedVertexFb* dst[3] = { reinterpret_cast< apemodefb::DecompressedVertexFb* >( m.vertices.data( ) + stride * ( i + 0 ) )
                                                      , reinterpret_cast< apemodefb::DecompressedVertexFb* >( m.vertices.data( ) + stride * ( i + 1 ) )
                                                      , reinterpret_cast< apemodefb::DecompressedVertexFb* >( m.vertices.data( ) + stride * ( i + 2 ) ) };

            builder.SetAttributeValuesForFace( positionAttributeIndex,
                                               faceIndex,
                                               reinterpret_cast< const float* >( &dst[ 0 ]->position( ) ),
                                               reinterpret_cast< const float* >( &dst[ 1 ]->position( ) ),
                                               reinterpret_cast< const float* >( &dst[ 2 ]->position( ) ) );
            builder.SetAttributeValuesForFace( uvAttributeIndex,
                                               faceIndex,
                                               reinterpret_cast< const float* >( &dst[ 0 ]->uv( ) ),
                                               reinterpret_cast< const float* >( &dst[ 1 ]->uv( ) ),
                                               reinterpret_cast< const float* >( &dst[ 2 ]->uv( ) ) );
            builder.SetAttributeValuesForFace( normalAttributeIndex,
                                               faceIndex,
                                               reinterpret_cast< const float* >( &dst[ 0 ]->normal( ) ),
                                               reinterpret_cast< const float* >( &dst[ 1 ]->normal( ) ),
                                               reinterpret_cast< const float* >( &dst[ 2 ]->normal( ) ) );
            builder.SetAttributeValuesForFace( tangentAttributeIndex,
                                               faceIndex,
                                               reinterpret_cast< const float* >( &dst[ 0 ]->tangent( ) ),
                                               reinterpret_cast< const float* >( &dst[ 1 ]->tangent( ) ),
                                               reinterpret_cast< const float* >( &dst[ 2 ]->tangent( ) ) );
            builder.SetAttributeValuesForFace( colorAttributeIndex,
                                               faceIndex,
                                               reinterpret_cast< const float* >( &dst[ 0 ]->color( ) ),
                                               reinterpret_cast< const float* >( &dst[ 1 ]->color( ) ),
                                               reinterpret_cast< const float* >( &dst[ 2 ]->color( ) ) );
            
            const uint8_t r0 = uint8_t( dst[ 0 ]->reflection_index_packed( ) );
            const uint8_t r1 = uint8_t( dst[ 1 ]->reflection_index_packed( ) );
            const uint8_t r2 = uint8_t( dst[ 2 ]->reflection_index_packed( ) );
            builder.SetAttributeValuesForFace( reflectionIndexAttributeIndex,
                                               faceIndex,
                                               reinterpret_cast< const float* >( &r0 ),
                                               reinterpret_cast< const float* >( &r1 ),
                                               reinterpret_cast< const float* >( &r2 ) );
       
            switch ( eVertexFmt ) {
            case apemodefb::EVertexFormatFb_Decompressed:
                break;
            case apemodefb::EVertexFormatFb_DecompressedSkinned: {
                apemodefb::DecompressedSkinnedVertexFb* skinnedDst[3] =
                { reinterpret_cast< apemodefb::DecompressedSkinnedVertexFb* >( dst[ 0 ] ),
                  reinterpret_cast< apemodefb::DecompressedSkinnedVertexFb* >( dst[ 1 ] ),
                  reinterpret_cast< apemodefb::DecompressedSkinnedVertexFb* >( dst[ 2 ] ) };
                
                const auto j0 = skinnedDst[ 0 ]->joint_indices( );
                const auto j1 = skinnedDst[ 1 ]->joint_indices( );
                const auto j2 = skinnedDst[ 2 ]->joint_indices( );
                builder.SetAttributeValuesForFace( jointIndicesAttributeIndex,
                                                   faceIndex,
                                                   reinterpret_cast< const float* >( &j0 ),
                                                   reinterpret_cast< const float* >( &j1 ),
                                                   reinterpret_cast< const float* >( &j2 ) );
                builder.SetAttributeValuesForFace( jointWeightsAttributeIndex,
                                                   faceIndex,
                                                   reinterpret_cast< const float* >( &skinnedDst[ 0 ]->joint_weights( ) ),
                                                   reinterpret_cast< const float* >( &skinnedDst[ 1 ]->joint_weights( ) ),
                                                   reinterpret_cast< const float* >( &skinnedDst[ 2 ]->joint_weights( ) ) );
            } break;
            case apemodefb::EVertexFormatFb_DecompressedFatSkinned: {
                apemodefb::DecompressedFatSkinnedVertexFb* skinnedDst[3] =
                { reinterpret_cast< apemodefb::DecompressedFatSkinnedVertexFb* >( dst[ 0 ] ),
                  reinterpret_cast< apemodefb::DecompressedFatSkinnedVertexFb* >( dst[ 1 ] ),
                  reinterpret_cast< apemodefb::DecompressedFatSkinnedVertexFb* >( dst[ 2 ] ) };
                
                const auto j0 = skinnedDst[ 0 ]->decompressed_skinned( ).joint_indices( );
                const auto j1 = skinnedDst[ 1 ]->decompressed_skinned( ).joint_indices( );
                const auto j2 = skinnedDst[ 2 ]->decompressed_skinned( ).joint_indices( );
                builder.SetAttributeValuesForFace( jointIndicesAttributeIndex,
                                                   faceIndex,
                                                   reinterpret_cast< const float* >( &j0 ),
                                                   reinterpret_cast< const float* >( &j1 ),
                                                   reinterpret_cast< const float* >( &j2 ) );
                builder.SetAttributeValuesForFace( jointWeightsAttributeIndex,
                                                   faceIndex,
                                                   reinterpret_cast< const float* >( &skinnedDst[ 0 ]->decompressed_skinned( ).joint_weights( ) ),
                                                   reinterpret_cast< const float* >( &skinnedDst[ 1 ]->decompressed_skinned( ).joint_weights( ) ),
                                                   reinterpret_cast< const float* >( &skinnedDst[ 2 ]->decompressed_skinned( ).joint_weights( ) ) );

                const auto ej0 = skinnedDst[ 0 ]->extra_joint_indices( );
                const auto ej1 = skinnedDst[ 1 ]->extra_joint_indices( );
                const auto ej2 = skinnedDst[ 2 ]->extra_joint_indices( );
                builder.SetAttributeValuesForFace( extraJointIndicesAttributeIndex,
                                                   faceIndex,
                                                   reinterpret_cast< const float* >( &ej0 ),
                                                   reinterpret_cast< const float* >( &ej1 ),
                                                   reinterpret_cast< const float* >( &ej2 ) );
                builder.SetAttributeValuesForFace( extraJointWeightsAttributeIndex,
                                                   faceIndex,
                                                   reinterpret_cast< const float* >( &skinnedDst[ 0 ]->extra_joint_weights( ) ),
                                                   reinterpret_cast< const float* >( &skinnedDst[ 1 ]->extra_joint_weights( ) ),
                                                   reinterpret_cast< const float* >( &skinnedDst[ 2 ]->extra_joint_weights( ) ) );
            } break;
            default:
                assert(false);
                break;
            }
        }
        
        apemode::Stopwatch sw;
        s.console->info( "Starting draco mesh finalization ..." );
        if ( auto finalizedMesh = builder.Finalize( ) ) {
            s.console->info( "Draco mesh finalization succeeded." );
            s.console->info( "Starting draco encoding ..." );
            
            #if 0
            draco::MeshCleanup cleanup;
            if ( cleanup( finalizedMesh.get( ), draco::MeshCleanupOptions( ) ) ) {
                s.console->info( "Succeeded" );
            } else {
                s.console->info( "Failed" );
            }
            #endif

            draco::EncoderBuffer encoderBuffer{};
            
            draco::ExpertEncoder encoder( *finalizedMesh.get( ) );
            encoder.Reset( draco::ExpertEncoder::OptionsType::CreateDefaultOptions( ) );
            encoder.SetEncodingMethod( encoderMethod );
            encoder.SetSpeedOptions( -1, -1 );
            encoder.SetAttributeQuantization(positionAttributeIndex, 16);
            encoder.SetAttributeQuantization(uvAttributeIndex, 16);
            encoder.SetAttributeQuantization(normalAttributeIndex, 8);
            encoder.SetAttributeQuantization(tangentAttributeIndex, 8);
            encoder.SetAttributeQuantization(colorAttributeIndex, 8);
            encoder.SetAttributeQuantization(reflectionIndexAttributeIndex, 8);
            
            if (jointIndicesAttributeIndex != -1) {
                assert( jointWeightsAttributeIndex != -1 );
                if (jointIndicesQuantizationBits != -1)
                    encoder.SetAttributeQuantization(jointIndicesAttributeIndex, jointIndicesQuantizationBits);
                if (jointWeightsQuantizationBits != -1)
                    encoder.SetAttributeQuantization(jointWeightsAttributeIndex, jointWeightsQuantizationBits);
            }
            if (extraJointIndicesAttributeIndex != -1) {
                assert( extraJointWeightsAttributeIndex != -1 );
                if (jointIndicesQuantizationBits != -1)
                    encoder.SetAttributeQuantization(extraJointIndicesAttributeIndex, jointIndicesQuantizationBits);
                if (jointWeightsQuantizationBits != -1)
                    encoder.SetAttributeQuantization(extraJointWeightsAttributeIndex, jointWeightsQuantizationBits);
            }
        
//            auto options = draco::EncoderOptionsBase< draco::GeometryAttribute::Type >::CreateDefaultOptions( );
//            options.SetGlobalBool( "split_mesh_on_seams", false );
//            draco::Encoder encoder;
//            encoder.Reset( options );
//            encoder.SetEncodingMethod( encoderMethod );
//            encoder.SetSpeedOptions( -1, -1 );
//            encoder.SetAttributeQuantization( draco::GeometryAttribute::POSITION, 16 );
//            encoder.SetAttributeQuantization( draco::GeometryAttribute::TEX_COORD, 16 );
//            encoder.SetAttributeQuantization( draco::GeometryAttribute::GENERIC, 30 );
//            encoder.SetAttributeQuantization( draco::GeometryAttribute::NORMAL, 8 );
//            const draco::Status encoderStatus = encoder.EncodeMeshToBuffer( *finalizedMesh.get( ), &encoderBuffer );
        
            const draco::Status encoderStatus = encoder.EncodeToBuffer( &encoderBuffer );
            if ( encoderStatus.code( ) == draco::Status::OK ) {
                size_t originalVertexSize = vertexCount * ( 4 * 3 + 4 * 2 + 6 * 4 + 4 * 4 );
            
                s.console->info( "DME\t{}\t{}\t{}%\t{}\t{}\t{}",
                                 originalVertexSize, // m.vertices.size( ),
                                 encoderBuffer.size( ),
                                 // ToPrettySizeString( m.vertices.size( ) ),
                                 // ToPrettySizeString( encoderBuffer.size( ) ),
                                 ( 100.0f * encoderBuffer.size( ) / originalVertexSize ), // m.vertices.size( ) ),
                                 vertexCount,
                                 apemodefb::EnumNameEVertexFormatFb(eVertexFmt),
                                 sw.ElapsedSeconds( ) );

                eCompressionType = apemodefb::ECompressionTypeFb_GoogleDraco3D;
                m.vertices.resize( encoderBuffer.size( ) );
                memcpy( m.vertices.data( ), encoderBuffer.data( ), encoderBuffer.size( ) );
                decltype( m.indices )( ).swap( m.indices );
            } else {
                s.console->info( "Failed: code = {}, error = {}", int( encoderStatus.code( ) ), encoderStatus.error_msg( ) );
                assert( false );
                exit(-1);
            }
        }
    } else {
        size_t stride = 0;
        size_t strideUnskinned = 0;
        
        strideUnskinned = sizeof( apemodefb::DefaultVertexFb );
        if ( skinInfos.empty( ) ) {
            stride = sizeof( apemodefb::DefaultVertexFb );
            eVertexFmt = apemodefb::EVertexFormatFb_Default;
        } else {
            assert( boneCount == eBoneCountPerControlPoint_4 ||
                    boneCount == eBoneCountPerControlPoint_8 );

            switch ( boneCount ) {
            case eBoneCountPerControlPoint_4:
                stride = sizeof( apemodefb::SkinnedVertexFb );
                eVertexFmt = apemodefb::EVertexFormatFb_Skinned;
                break;
            case eBoneCountPerControlPoint_8:
                stride = sizeof( apemodefb::FatSkinnedVertexFb );
                eVertexFmt = apemodefb::EVertexFormatFb_FatSkinned;
                break;
            }
        }
        
        assert( stride != 0 );
        m.vertices.resize( stride * vertexCount );

        for ( uint32_t i = 0; i < vertexCount; ++i ) {
            union {
                struct {
                    uint8_t i;
                    uint8_t r;
                    uint8_t g;
                    uint8_t b;
                };

                uint32_t irgb;
            } p;

            p.i = uint8_t( i % 3 );
            p.r = uint8_t( vertices[ i ].color.x * 255.0 );
            p.g = uint8_t( vertices[ i ].color.y * 255.0 );
            p.b = uint8_t( vertices[ i ].color.z * 255.0 );

            float alpha = float( vertices[ i ].color.w );

            assert( ( stride * i ) < m.vertices.size( ) );
            auto& dst = *reinterpret_cast< apemodefb::DefaultVertexFb* >( m.vertices.data( ) + stride * i );
            
            dst.mutable_position( ).mutate_x( vertices[ i ].position.x );
            dst.mutable_position( ).mutate_y( vertices[ i ].position.y );
            dst.mutable_position( ).mutate_z( vertices[ i ].position.z );
            dst.mutate_index_color_RGB( p.irgb );
            dst.mutate_color_alpha( alpha );
            dst.mutable_uv( ).mutate_x( vertices[ i ].texCoords.x );
            dst.mutable_uv( ).mutate_y( vertices[ i ].texCoords.y );
            dst.mutable_qtangent( ).mutate_nx( qtangents[ i ].vector( ).x );
            dst.mutable_qtangent( ).mutate_ny( qtangents[ i ].vector( ).y );
            dst.mutable_qtangent( ).mutate_nz( qtangents[ i ].vector( ).z );
            dst.mutable_qtangent( ).mutate_s( qtangents[ i ].scalar( ) );
        }

        if ( !skinInfos.empty( ) ) {
            // auto & skin = s.skins[ m.skinId ];
            for ( uint32_t i = 0; i < vertexCount; ++i ) {
                auto controlPointIndex = vertices[ i ].controlPointIndex;
                auto& skinInfo = skinInfos[ controlPointIndex ];
                
                assert( boneCount == eBoneCountPerControlPoint_4 ||
                        boneCount == eBoneCountPerControlPoint_8 );

                switch ( boneCount ) {
                case eBoneCountPerControlPoint_4: {
                    const auto compiledWeightsIndices = skinInfo.Compile< eBoneCountPerControlPoint_4 >( );
                    
                    auto dst = reinterpret_cast< apemodefb::SkinnedVertexFb* >( m.vertices.data( ) + stride * i );
                    dst->mutable_joint_indices_weights( ).mutate_x( compiledWeightsIndices[ 0 ] );
                    dst->mutable_joint_indices_weights( ).mutate_y( compiledWeightsIndices[ 1 ] );
                    dst->mutable_joint_indices_weights( ).mutate_z( compiledWeightsIndices[ 2 ] );
                    dst->mutable_joint_indices_weights( ).mutate_w( compiledWeightsIndices[ 3 ] );
                } break;
                case eBoneCountPerControlPoint_8: {
                    const auto compiledWeightsIndices = skinInfo.Compile< eBoneCountPerControlPoint_8 >( );
                    
                    auto dst = reinterpret_cast< apemodefb::FatSkinnedVertexFb* >( m.vertices.data( ) + stride * i );
                    dst->mutable_skinned_vertex( ).mutable_joint_indices_weights( ).mutate_x( compiledWeightsIndices[ 0 ] );
                    dst->mutable_skinned_vertex( ).mutable_joint_indices_weights( ).mutate_y( compiledWeightsIndices[ 1 ] );
                    dst->mutable_skinned_vertex( ).mutable_joint_indices_weights( ).mutate_z( compiledWeightsIndices[ 2 ] );
                    dst->mutable_skinned_vertex( ).mutable_joint_indices_weights( ).mutate_w( compiledWeightsIndices[ 3 ] );
                    dst->mutable_extra_joint_indices_weights( ).mutate_x( compiledWeightsIndices[ 4 ] );
                    dst->mutable_extra_joint_indices_weights( ).mutate_y( compiledWeightsIndices[ 5 ] );
                    dst->mutable_extra_joint_indices_weights( ).mutate_z( compiledWeightsIndices[ 6 ] );
                    dst->mutable_extra_joint_indices_weights( ).mutate_w( compiledWeightsIndices[ 7 ] );
                } break;
                }
            }
        }
    }

    assert( eVertexFmt != apemodefb::EVertexFormatFb( -1 ) );
    const apemodefb::Vec3Fb bboxMin( m.positionMin.x(), m.positionMin.y(), m.positionMin.z() );
    const apemodefb::Vec3Fb bboxMax( m.positionMax.x(), m.positionMax.y(), m.positionMax.z() );

    m.submeshes.emplace_back( bboxMin,                               // bbox min
                              bboxMax,                               // bbox max
                              0,                                     // base vertex
                              vertexCount,                           // vertex count
                              0,                                     // base index
                              0,                                     // index count
                              0,                                     // base subset
                              (uint32_t) m.subsets.size( ),          // subset count
                              eVertexFmt,                            // vertex format
                              eCompressionType                       // compression
    );
}

void ExportMesh( FbxNode* node, apemode::Node& n, bool pack, bool optimize ) {
    auto& s = apemode::State::Get( );
    if ( auto mesh = node->GetMesh( ) ) {

        s.console->info( "Node \"{}\" has mesh.", node->GetName( ) );
        if ( !mesh->IsTriangleMesh( ) ) {

            s.console->warn( "Mesh \"{}\" is not triangular, processing...", node->GetName( ) );
            FbxGeometryConverter converter( mesh->GetNode( )->GetFbxManager( ) );
            mesh = (FbxMesh*) converter.Triangulate( mesh, true, s.legacyTriangulationSdk );

            if ( nullptr == mesh ) {
                s.console->error( "Mesh \"{}\" triangulation failed (mesh will be skipped).", node->GetName( ) );
            } else {
                s.console->warn( "Mesh \"{}\" was triangulated (success).", node->GetName( ) );
            }
        }

        if ( nullptr != mesh ) {

            if ( const auto deformerCount = mesh->GetDeformerCount( ) - mesh->GetDeformerCount( FbxDeformer::eSkin ) ) {
                s.console->warn( "Mesh \"{}\" has {} non-skin deformers (will be ignored).", node->GetName( ), deformerCount );
            }

            if ( const auto skinCount = mesh->GetDeformerCount( FbxDeformer::eSkin ) ) {
                if ( skinCount > 1 ) {
                    s.console->warn( "Mesh \"{}\" has {} skin deformers (only one will be included).", node->GetName( ), skinCount );
                }
            }

            if ( const uint32_t vertexCount = mesh->GetPolygonCount( ) * 3 ) {
                n.meshId = (uint32_t) s.meshes.size( );
                s.meshes.emplace_back( );
                apemode::Mesh& m = s.meshes.back( );

                FbxSkin* pSkin = 0 != mesh->GetDeformerCount( FbxDeformer::eSkin )
                                     ? FbxCast< FbxSkin >( mesh->GetDeformer( 0, FbxDeformer::eSkin ) )
                                     : nullptr;

                if ( vertexCount < std::numeric_limits< uint16_t >::max( ) )
                    ExportMesh< uint16_t >( node, mesh, n, m, vertexCount, pack, pSkin, optimize );
                else
                    ExportMesh< uint32_t >( node, mesh, n, m, vertexCount, pack, pSkin, optimize );

            } else {
                s.console->error( "Mesh \"{}\" has no vertices (skipped).", node->GetName( ) );
            }
        }
    }
}
