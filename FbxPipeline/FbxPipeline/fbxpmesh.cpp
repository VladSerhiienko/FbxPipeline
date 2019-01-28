#include <fbxppch.h>
#include <fbxpstate.h>
#include <fbxpnorm.h>
#include <map>
#include <array>

#include <draco/mesh/mesh.h>
#include <draco/mesh/mesh_cleanup.h>
#include <draco/mesh/triangle_soup_mesh_builder.h>

#include <draco/compression/encode.h>
#include <draco/compression/decode.h>

/**
 * Helper structure to assign vertex property values
 **/
struct StaticVertex {
    mathfu::vec3 position;
    mathfu::vec3 normal;
    mathfu::vec4 tangent;
    mathfu::vec2 texCoords;
};

template <typename T>
void AssertValidVec3(T values) {
    assert( !isnan( values[0] ) && !isnan( values[1] ) && !isnan( values[2] ) );
    assert( !isinf( values[0] ) && !isinf( values[1] ) && !isinf( values[2] ) );
}

bool CalculateTangentsNoUVs( StaticVertex* vertices, size_t vertexCount ) {
    std::vector< mathfu::vec3 > tan;
    tan.resize( vertexCount * 2 );

    auto tan1 = tan.data( );
    auto tan2 = tan1 + vertexCount;

    for ( size_t i = 0; i < vertexCount; i += 3 ) {
        const StaticVertex v0 = vertices[ i + 0 ];
        const StaticVertex v1 = vertices[ i + 1 ];
        const StaticVertex v2 = vertices[ i + 2 ];
        
        mathfu::vec3 e1 = v1.position - v0.position;
        mathfu::vec3 e2 = v2.position - v0.position;
        e1.Normalize();
        e2.Normalize();
        
        AssertValidVec3(e1);
        AssertValidVec3(e2);
        
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
        v.tangent = mathfu::kZeros4f;
        v.tangent.w = 1;
        
        const auto n = v.normal.Normalized();
        mathfu::vec3 t = tan1[ i ];
        AssertValidVec3(n);
        AssertValidVec3(t);

        const bool isOk = n.Length() && t.Length();
        result &= isOk;
        if ( isOk ) {
            mathfu::vec3 tt = mathfu::normalize( t - n * mathfu::dot( n, t ) );
            AssertValidVec3(tt);

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
    std::vector< mathfu::vec3 > tan;
    tan.resize( vertexCount * 2 );

    auto tan1 = tan.data( );
    auto tan2 = tan1 + vertexCount;

    for ( size_t i = 0; i < vertexCount; i += 3 ) {
        const StaticVertex v0 = vertices[ i + 0 ];
        const StaticVertex v1 = vertices[ i + 1 ];
        const StaticVertex v2 = vertices[ i + 2 ];

        const float x1 = v1.position[ 0 ] - v0.position[ 0 ];
        const float x2 = v2.position[ 0 ] - v0.position[ 0 ];
        const float y1 = v1.position[ 1 ] - v0.position[ 1 ];
        const float y2 = v2.position[ 1 ] - v0.position[ 1 ];
        const float z1 = v1.position[ 2 ] - v0.position[ 2 ];
        const float z2 = v2.position[ 2 ] - v0.position[ 2 ];

        const float s1 = v1.texCoords[ 0 ] - v0.texCoords[ 0 ];
        const float s2 = v2.texCoords[ 0 ] - v0.texCoords[ 0 ];
        const float t1 = v1.texCoords[ 1 ] - v0.texCoords[ 1 ];
        const float t2 = v2.texCoords[ 1 ] - v0.texCoords[ 1 ];

        const float r = 1.0f / ( ( s1 * t2 - s2 * t1 ) + std::numeric_limits<float>::epsilon( ) );
        const mathfu::vec3 sdir( ( t2 * x1 - t1 * x2 ) * r, ( t2 * y1 - t1 * y2 ) * r, ( t2 * z1 - t1 * z2 ) * r );
        const mathfu::vec3 tdir( ( s1 * x2 - s2 * x1 ) * r, ( s1 * y2 - s2 * y1 ) * r, ( s1 * z2 - s2 * z1 ) * r );

        AssertValidVec3(sdir);
        AssertValidVec3(tdir);

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
        v.tangent = mathfu::kZeros4f;
        v.tangent.w = 1;
        
        const auto n = v.normal.Normalized();
        mathfu::vec3 t = tan1[ i ];
        AssertValidVec3(n);
        AssertValidVec3(t);

        const bool isOk = n.Length() && t.Length();
        result &= isOk;
        if ( isOk ) {
            mathfu::vec3 tt = mathfu::normalize( t - n * mathfu::dot( n, t ) );
            AssertValidVec3(tt);

            v.tangent[ 0 ]  = tt.x;
            v.tangent[ 1 ]  = tt.y;
            v.tangent[ 2 ]  = tt.z;
            v.tangent[ 3 ]  = ( mathfu::dot( mathfu::cross( n, t ), tan2[ i ] ) < 0 ) ? -1.0f : 1.0f;
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

        const mathfu::vec3 p0( v0.position );
        const mathfu::vec3 p1( v1.position );
        const mathfu::vec3 p2( v2.position );
        const mathfu::vec3 n( mathfu::normalize( mathfu::cross( p1 - p0, p2 - p0 ) ) );

        AssertValidVec3(p0);
        AssertValidVec3(p1);
        AssertValidVec3(p2);
        AssertValidVec3(n);

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
                                if ( directArray && directArray->GetCount( ) < 1 ) {
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
                                if ( indexArray && indexArray->GetCount( ) < 1 ) {
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
struct StaticSkinnedVertex {
    mathfu::vec3 position;
    mathfu::vec3 normal;
    mathfu::vec4 tangent;
    mathfu::vec2 texCoords;
    mathfu::vec4 weights;
    mathfu::vec4 indices;
};

/**
 * Helper structure to assign vertex property values
 **/
struct StaticSkinned8Vertex {
    mathfu::vec3 position;
    mathfu::vec3 normal;
    mathfu::vec4 tangent;
    mathfu::vec2 texCoords;
    mathfu::vec4 weights_0;
    mathfu::vec4 weights_1;
    mathfu::vec4 indices_0;
    mathfu::vec4 indices_1;
};

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
};

//
// Flatbuffers takes care about correct platform-independent alignment.
//

//static_assert( sizeof( StaticVertex ) == sizeof( apemodefb::StaticVertexFb ), "Must match" );
//static_assert( sizeof( StaticSkinnedVertex ) == sizeof( apemodefb::StaticSkinnedVertexFb ), "Must match" );
//static_assert( sizeof( StaticSkinned8Vertex ) == sizeof( apemodefb::StaticSkinned8VertexFb ), "Must match" );

bool IsValidNormal(FbxVector4 values) {
    if ((isnan(values[0]) || isinf(values[0]))
        || (isnan(values[1]) || isinf(values[1]))
        || (isnan(values[2]) || isinf(values[2]))) {
        return false;
    }
    
    const double length = values.Length();
    if (length < std::numeric_limits<float>::epsilon()) {
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
    auto ne  = VerifyElementLayer( mesh->GetElementNormal( ) );
    auto te  = VerifyElementLayer( mesh->GetElementTangent( ) );

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
            
            n.Normalize();
            t.Normalize();
            
            AssertValidVec3(cp);
            AssertValidVec3(n);
            AssertValidVec3(t);
            AssertValidVec3(uv);
            
            if (ne && !IsValidNormal(n)) {
                ne = nullptr;
                s.console->warn( "Mesh \"{}\" has invalid normals", mesh->GetNode( )->GetName( ) );
                s.console->warn( "Mesh \"{}\" will have generated normals", mesh->GetNode( )->GetName( ) );
            }
            if (te && !IsValidNormal(t)) {
                te = nullptr;
                s.console->warn( "Mesh \"{}\" has invalid tangents", mesh->GetNode( )->GetName( ) );
                s.console->warn( "Mesh \"{}\" will have generated tangents", mesh->GetNode( )->GetName( ) );
            }

            auto& vvii          = vertices[ vi ];
            vvii.position[ 0 ]  = (float) cp[ 0 ];
            vvii.position[ 1 ]  = (float) cp[ 1 ];
            vvii.position[ 2 ]  = (float) cp[ 2 ];
            vvii.normal[ 0 ]    = (float) n[ 0 ];
            vvii.normal[ 1 ]    = (float) n[ 1 ];
            vvii.normal[ 2 ]    = (float) n[ 2 ];
            vvii.tangent[ 0 ]   = (float) t[ 0 ];
            vvii.tangent[ 1 ]   = (float) t[ 1 ];
            vvii.tangent[ 2 ]   = (float) t[ 2 ];
            vvii.tangent[ 3 ]   = (float) t[ 3 ];
            vvii.texCoords[ 0 ] = (float) uv[ 0 ];
            vvii.texCoords[ 1 ] = (float) uv[ 1 ];
            
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
            
            const mathfu::vec3 nn = vvii.normal.Normalized();
            const mathfu::vec3 tt = vvii.tangent.xyz().Normalized();
            const mathfu::vec3 ttt = mathfu::normalize( tt - nn * mathfu::dot( nn, tt ) );
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
            const mathfu::vec3 nn = vvii.normal.Normalized();
            
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
void Optimize32( apemode::Mesh& mesh, apemodefb::StaticVertexFb const* vertices, uint32_t & vertexCount, uint32_t vertexStride );
void Optimize16( apemode::Mesh& mesh, apemodefb::StaticVertexFb const* vertices, uint32_t & vertexCount, uint32_t vertexStride );

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

    std::vector< mathfu::quat > qtangents;
    if ( initResult.bValidTangents && s.options[ "tangent-frame-format" ].count( ) ) {
        assert( s.options[ "tangent-frame-format" ].as< std::string >( ) == "quat-float" );

        qtangents.resize( vertexCount );
        for ( uint32_t i = 0; i < vertexCount; ++i ) {
            const mathfu::vec3 n( vertices[ i ].normal );
            const mathfu::vec3 t( vertices[ i ].tangent.xyz( ) );
            const mathfu::vec3 b( mathfu::cross( n, t ) );
            const mathfu::mat3 m( t.x, t.y, t.z, b.x, b.y, b.z, n.x, n.y, n.z );

            mathfu::quat q( mathfu::quat::FromMatrix( m ).Normalized( ) );

            // TODO: Should be in use for packing it into ints
            //       because the sign will be lost.
            const float bias = float( 1.0 / ( pow( 2.0, 15.0 ) - 1.0 ) );
            if ( q.scalar( ) < bias ) {
                float s = q.scalar( );
                mathfu::vec3 v = q.vector( );
                s = bias;
                v *= sqrt( 1.0 - bias * bias );
                q = mathfu::quat( s, v ).Normalized( );
            }

            if ( q.scalar( ) < 0.0f ) {
                q = mathfu::quat( -q.scalar( ), -q.vector( ) );
            }

            const float reflection = vertices[ i ].tangent.w;
            if ( reflection < 0.0f ) {
                q = mathfu::quat( -q.scalar( ), -q.vector( ) );
            }
            
            q = q.Normalized( );
            
            assert( !isnan( q.scalar( ) ) );
            assert( !isnan( q.vector( ).x ) );
            assert( !isnan( q.vector( ).y ) );
            assert( !isnan( q.vector( ).z ) );
            assert( !isinf( q.scalar( ) ) );
            assert( !isinf( q.vector( ).x ) );
            assert( !isinf( q.vector( ).y ) );
            assert( !isinf( q.vector( ).z ) );

            qtangents[ i ] = q;
        }
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

                const FbxVector4 T = invBindPoseMatrix.GetT( );
                const FbxQuaternion Q = invBindPoseMatrix.GetQ( );

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

    size_t stride = 0;
    size_t strideUnskinned = 0;
    apemodefb::EVertexFormatFb eVertexFmt;

    if ( skinInfos.empty( ) ) {
        if ( qtangents.empty( ) ) {
            stride = sizeof( apemodefb::StaticVertexFb );
            eVertexFmt = apemodefb::EVertexFormatFb_Static;
        } else {
            stride = sizeof( apemodefb::StaticVertexQTangentFb );
            eVertexFmt = apemodefb::EVertexFormatFb_StaticQTangent;
        }
        strideUnskinned = stride;
    } else {
        assert( boneCount == eBoneCountPerControlPoint_4 || boneCount == eBoneCountPerControlPoint_8 );
        if ( qtangents.empty( ) ) {
            strideUnskinned = sizeof( apemodefb::StaticVertexFb );
            switch ( boneCount ) {
                case eBoneCountPerControlPoint_4:
                    stride = sizeof( apemodefb::StaticSkinnedVertexFb );
                    eVertexFmt = apemodefb::EVertexFormatFb_StaticSkinned;
                    break;
                case eBoneCountPerControlPoint_8:
                    stride = sizeof( apemodefb::StaticSkinned8VertexFb );
                    eVertexFmt = apemodefb::EVertexFormatFb_StaticSkinned8;
                    break;
            }
        } else {
            strideUnskinned = sizeof( apemodefb::StaticVertexQTangentFb );
            switch ( boneCount ) {
                case eBoneCountPerControlPoint_4:
                    stride = sizeof( apemodefb::StaticSkinnedVertexQTangentFb );
                    eVertexFmt = apemodefb::EVertexFormatFb_StaticSkinnedQTangent;
                    break;
                case eBoneCountPerControlPoint_8:
                    stride = sizeof( apemodefb::StaticSkinned8VertexQTangentFb );
                    eVertexFmt = apemodefb::EVertexFormatFb_StaticSkinned8QTangent;
                    break;
            }
        }
    }

    assert( stride != 0 );
    m.vertices.resize( stride * vertexCount );

    if ( !qtangents.empty( ) ) {
        for ( uint32_t i = 0; i < vertexCount; ++i ) {
            assert( ( stride * i ) < m.vertices.size( ) );
            auto& dst = *reinterpret_cast< apemodefb::StaticVertexQTangentFb* >( m.vertices.data( ) + stride * i );
            dst.mutable_position( ).mutate_x( vertices[ i ].position.x );
            dst.mutable_position( ).mutate_y( vertices[ i ].position.y );
            dst.mutable_position( ).mutate_z( vertices[ i ].position.z );
            dst.mutable_uv( ).mutate_x( vertices[ i ].texCoords.x );
            dst.mutable_uv( ).mutate_y( vertices[ i ].texCoords.y );
            dst.mutable_qtangent( ).mutate_s( qtangents[ i ].scalar( ) );
            dst.mutable_qtangent( ).mutate_nx( qtangents[ i ].vector( ).x );
            dst.mutable_qtangent( ).mutate_ny( qtangents[ i ].vector( ).y );
            dst.mutable_qtangent( ).mutate_nz( qtangents[ i ].vector( ).z );
        }
    } else {
        for ( uint32_t i = 0; i < vertexCount; ++i ) {
            assert( ( stride * i ) < m.vertices.size( ) );
            auto& dst = *reinterpret_cast< apemodefb::StaticVertexFb* >( m.vertices.data( ) + stride * i );
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
            dst.mutable_tangent( ).mutate_w( vertices[ i ].tangent.w );
        }
    }

    if (!skinInfos.empty()) {
        auto & skin = s.skins[m.skinId];
        (void)skin;
    
        uint32_t vertexIndex = 0;
        for ( int polygonIndex = 0; polygonIndex < pMesh->GetPolygonCount( ); ++polygonIndex ) {
            for ( const int polygonVertexIndex : {0, 1, 2} ) {
                const int controlPointIndex = pMesh->GetPolygonVertex( polygonIndex, polygonVertexIndex );
                const auto& weightsIndices = skinInfos[ controlPointIndex ].weights;
                
                /*
                weights_0 : Vec4Fb;
                indices_0 : Vec4Fb;
                
                weights_0 : Vec4Fb;
                weights_1 : Vec4Fb;
                indices_0 : Vec4Fb;
                indices_1 : Vec4Fb;
                */

                float* dst = reinterpret_cast< float* >( m.vertices.data( ) + stride * vertexIndex + strideUnskinned );
                for ( uint32_t b = 0; b < eBoneCountPerControlPoint_4; ++b ) {
                    dst[ b ] = weightsIndices[ b ].weight;

                    assert( weightsIndices[ b ].index < skin.linkIds.size( ) );
                    if ( boneCount == eBoneCountPerControlPoint_4 ) {
                        dst[ eBoneCountPerControlPoint_4 + b ] = float( weightsIndices[ b ].index );
                    } else {
                        assert( boneCount == eBoneCountPerControlPoint_8 );
                        dst[ eBoneCountPerControlPoint_4 * 2 + b ] = float( weightsIndices[ b ].index );
                    }
                }

                if ( boneCount == eBoneCountPerControlPoint_8 ) {
                    for ( uint32_t b = 0; b < eBoneCountPerControlPoint_4; ++b ) {
                        dst[ eBoneCountPerControlPoint_4 + b ] = weightsIndices[ eBoneCountPerControlPoint_4 + b ].weight;
                        dst[ eBoneCountPerControlPoint_4 * 3 + b ] = float( weightsIndices[ eBoneCountPerControlPoint_4 + b ].index );
                    }
                }

                ++vertexIndex;
            }
        }
    }
    
    apemodefb::ECompressionTypeFb eCompressionType = apemodefb::ECompressionTypeFb_None;

    const std::string& meshCompression = s.options[ "mesh-compression" ].as< std::string >( );
    if ( s.options[ "mesh-compression" ].count() && meshCompression != "none" ) {
        switch ( eVertexFmt ) {
            case apemodefb::EVertexFormatFb_Static:
            case apemodefb::EVertexFormatFb_StaticQTangent: {
                draco::MeshEncoderMethod encoderMethod = draco::MESH_EDGEBREAKER_ENCODING;
                if ( meshCompression != "draco-edgebreaker" ) {
                    assert( meshCompression == "draco-esequential" );
                    encoderMethod = draco::MESH_SEQUENTIAL_ENCODING;
                }

                draco::Encoder encoder;
                auto           options = draco::EncoderOptionsBase< draco::GeometryAttribute::Type >::CreateDefaultOptions( );
                options.SetGlobalBool( "split_mesh_on_seams", false );

                encoder.Reset( options );
                encoder.SetEncodingMethod( encoderMethod );
                encoder.SetSpeedOptions( -1, -1 );
                encoder.SetAttributeQuantization( draco::GeometryAttribute::POSITION, 8 );
                encoder.SetAttributeQuantization( draco::GeometryAttribute::TEX_COORD, 8 );
                encoder.SetAttributeQuantization( draco::GeometryAttribute::GENERIC, 8 );
                encoder.SetAttributeQuantization( draco::GeometryAttribute::NORMAL, 8 );

                draco::TriangleSoupMeshBuilder builder;
                assert( vertexCount % 3 == 0 );
                builder.Start( vertexCount / 3 );

                // clang-format off
                if ( eVertexFmt == apemodefb::EVertexFormatFb_Static ) {
                    const int positionAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::POSITION, 3, draco::DataType::DT_FLOAT32 );
                    const int normalAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::NORMAL, 3, draco::DataType::DT_FLOAT32 );
                    const int tangentAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::NORMAL, 3, draco::DataType::DT_FLOAT32 );
                    const int reflectionAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::GENERIC, 1, draco::DataType::DT_FLOAT32 );
                    const int texCoordAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::TEX_COORD, 2, draco::DataType::DT_FLOAT32 );

                    auto pVertices = reinterpret_cast< const apemodefb::StaticVertexFb* >( m.vertices.data( ) );
                    for ( uint32_t i = 0; i < vertexCount; i += 3 ) {
                        const draco::FaceIndex faceIndex = draco::FaceIndex( i / 3 );

                        builder.SetAttributeValuesForFace( positionAttributeIndex,
                                                           faceIndex,
                                                           reinterpret_cast< const float* >( &pVertices[ i + 0 ].position( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 1 ].position( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 2 ].position( ) ) );
                        builder.SetAttributeValuesForFace( normalAttributeIndex,
                                                           faceIndex,
                                                           reinterpret_cast< const float* >( &pVertices[ i + 0 ].normal( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 1 ].normal( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 2 ].normal( ) ) );
                        builder.SetAttributeValuesForFace( tangentAttributeIndex,
                                                           faceIndex,
                                                           reinterpret_cast< const float* >( &pVertices[ i + 0 ].tangent( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 1 ].tangent( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 2 ].tangent( ) ) );
                        builder.SetAttributeValuesForFace( reflectionAttributeIndex,
                                                           faceIndex,
                                                           reinterpret_cast< const float* >( &pVertices[ i + 0 ].tangent( ) ) + 3,
                                                           reinterpret_cast< const float* >( &pVertices[ i + 1 ].tangent( ) ) + 3,
                                                           reinterpret_cast< const float* >( &pVertices[ i + 2 ].tangent( ) ) + 3 );
                        builder.SetAttributeValuesForFace( texCoordAttributeIndex,
                                                           faceIndex,
                                                           reinterpret_cast< const float* >( &pVertices[ i + 0 ].uv( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 1 ].uv( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 2 ].uv( ) ) );
                    }
                } else {
                    assert( eVertexFmt == apemodefb::EVertexFormatFb_StaticQTangent );
                    const int positionAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::POSITION, 3, draco::DataType::DT_FLOAT32 );
                    const int qtangentAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::GENERIC, 4, draco::DataType::DT_FLOAT32 );
                    const int texCoordAttributeIndex = builder.AddAttribute( draco::GeometryAttribute::Type::TEX_COORD, 2, draco::DataType::DT_FLOAT32 );

                    auto pVertices = reinterpret_cast< const apemodefb::StaticVertexQTangentFb* >( m.vertices.data( ) );
                    for ( uint32_t i = 0; i < vertexCount; i += 3 ) {
                        const draco::FaceIndex faceIndex = draco::FaceIndex( i / 3 );

                        builder.SetAttributeValuesForFace( positionAttributeIndex,
                                                           faceIndex,
                                                           reinterpret_cast< const float* >( &pVertices[ i + 0 ].position( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 1 ].position( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 2 ].position( ) ) );
                        builder.SetAttributeValuesForFace( qtangentAttributeIndex,
                                                           faceIndex,
                                                           reinterpret_cast< const float* >( &pVertices[ i + 0 ].qtangent( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 1 ].qtangent( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 2 ].qtangent( ) ) );
                        builder.SetAttributeValuesForFace( texCoordAttributeIndex,
                                                           faceIndex,
                                                           reinterpret_cast< const float* >( &pVertices[ i + 0 ].uv( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 1 ].uv( ) ),
                                                           reinterpret_cast< const float* >( &pVertices[ i + 2 ].uv( ) ) );
                    }
                }
                // clang-format on

                if ( auto finalizedMesh = builder.Finalize( ) ) {
                    draco::MeshCleanup cleanup;
                    s.console->info( "Starting draco cleanup ..." );
                    if ( cleanup( finalizedMesh.get( ), draco::MeshCleanupOptions( ) ) ) {
                        s.console->info( "Succeeded" );
                    } else {
                        s.console->info( "Failed" );
                    }

                    s.console->info( "Starting draco encoding ..." );
                    draco::EncoderBuffer encoderBuffer;
                    const draco::Status  encoderStatus = encoder.EncodeMeshToBuffer( *finalizedMesh.get( ), &encoderBuffer );
                    if ( encoderStatus.code( ) == draco::Status::OK ) {
                        s.console->info( "Succeeded: {} -> {}, {}%",
                                         ToPrettySizeString( m.vertices.size( ) ),
                                         ToPrettySizeString( encoderBuffer.size( ) ),
                                         ( 100.0f * encoderBuffer.size( ) / m.vertices.size( ) ) );

                        eCompressionType = apemodefb::ECompressionTypeFb_GoogleDraco3D;
                        m.vertices.resize( encoderBuffer.size( ) );
                        memcpy( m.vertices.data( ), encoderBuffer.data( ), encoderBuffer.size( ) );

                        decltype( m.indices ) emptyIndices;
                        m.indices.swap( emptyIndices );
                    } else {
                        s.console->info( "Failed: code = {}, error = {}", int( encoderStatus.code( ) ), encoderStatus.error_msg( ) );
                    }
                }
            } break;
            default:
                break;
        }
    }
    
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
