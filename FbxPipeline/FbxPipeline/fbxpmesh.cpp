#include <fbxppch.h>
#include <fbxpstate.h>
#include <fbxpnorm.h>
#include <map>

/**
 * Helper function to calculate tangents when the tangent element layer is missing.
 * Can be used in multiple threads.
 * http://gamedev.stackexchange.com/a/68617/39505
 **/
template < typename TVertex >
void CalculateTangents( TVertex* vertices, size_t vertexCount ) {
    std::vector< mathfu::vec3 > tan;
    tan.resize( vertexCount * 2 );
    memset( tan.data( ), 0, sizeof( mathfu::vec3 ) * vertexCount * 2 );

    auto tan1 = tan.data( );
    auto tan2 = tan1 + vertexCount;

    for ( size_t i = 0; i < vertexCount; i += 3 ) {
        const TVertex v0 = vertices[ i + 0 ];
        const TVertex v1 = vertices[ i + 1 ];
        const TVertex v2 = vertices[ i + 2 ];

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

        const float r = 1.0f / ( s1 * t2 - s2 * t1 );
        const mathfu::vec3 sdir( ( t2 * x1 - t1 * x2 ) * r, ( t2 * y1 - t1 * y2 ) * r, ( t2 * z1 - t1 * z2 ) * r );
        const mathfu::vec3 tdir( ( s1 * x2 - s2 * x1 ) * r, ( s1 * y2 - s2 * y1 ) * r, ( s1 * z2 - s2 * z1 ) * r );

        // assert( !isnan( sdir.x ) && !isnan( sdir.y ) && !isnan( sdir.z ) );
        // assert( !isnan( tdir.x ) && !isnan( tdir.y ) && !isnan( tdir.z ) );

        tan1[ i + 0 ] += sdir;
        tan1[ i + 1 ] += sdir;
        tan1[ i + 2 ] += sdir;

        tan2[ i + 0 ] += tdir;
        tan2[ i + 1 ] += tdir;
        tan2[ i + 2 ] += tdir;
    }

    for ( size_t i = 0; i < vertexCount; i += 1 ) {
        TVertex& v = vertices[ i ];
        const auto n = mathfu::vec3( v.normal[ 0 ], v.normal[ 1 ], v.normal[ 2 ] );
        const auto t = tan1[ i ];

        mathfu::vec3 tt = mathfu::normalize( t - n * mathfu::dot( n, t ) );
        // assert( !isnan( tt.x ) && !isnan( tt.y ) && !isnan( tt.z ) );

        v.tangent[ 0 ]  = tt.x;
        v.tangent[ 1 ]  = tt.y;
        v.tangent[ 2 ]  = tt.z;
        v.tangent[ 3 ]  = ( mathfu::dot( mathfu::cross( n, t ), tan2[ i ] ) < 0 ) ? -1.0f : 1.0f;
    }
}

/**
 * Calculate normals for the faces (does not weight triangles).
 * Fast and usable results, however incorrect.
 * TODO: Implement vertex normal calculations.
 **/
template < typename TVertex >
void CalculateFaceNormals( TVertex* vertices, size_t vertexCount ) {
    for ( size_t i = 0; i < vertexCount; i += 3 ) {
        TVertex& v0 = vertices[ i + 0 ];
        TVertex& v1 = vertices[ i + 1 ];
        TVertex& v2 = vertices[ i + 2 ];

        const mathfu::vec3 p0( v0.position );
        const mathfu::vec3 p1( v1.position );
        const mathfu::vec3 p2( v2.position );
        const mathfu::vec3 n( mathfu::normalize( mathfu::cross( p1 - p0, p2 - p0 ) ) );

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
struct StaticVertex {
    float position[ 3 ];
    float normal[ 3 ];
    float tangent[ 4 ];
    float texCoords[ 2 ];
};

/**
 * Helper structure to assign vertex property values
 **/
struct StaticSkinnedVertex {
    float position[ 3 ];
    float normal[ 3 ];
    float tangent[ 4 ];
    float texCoords[ 2 ];
    float weights[ 4 ];
    float indices[ 4 ];
};

using BoneWeightType = float;
static const uint32_t sInvalidIndex = uint32_t( -1 );

enum EBoneCountPerControlPoint : uint32_t {
    eBoneCountPerControlPoint_4 = 4,
    eBoneCountPerControlPoint_8 = 8,
};

template < uint32_t TBoneCountPerControlPoint = eBoneCountPerControlPoint_4 >
struct TControlPointSkinInfo {

    static const uint32_t kBoneCountPerControlPoint = TBoneCountPerControlPoint;

    BoneWeightType weights[ kBoneCountPerControlPoint ];
    uint32_t       indices[ kBoneCountPerControlPoint ];

    TControlPointSkinInfo( ) {
        for ( uint32_t i = 0; i < kBoneCountPerControlPoint; ++i ) {
            weights[ i ] = 0.0f;
            indices[ i ] = sInvalidIndex;
        }
    }

    /** Assignes bone to the control point.
     * @param weight Bone influence weight.
     * @param index Bone index.
     * @return true of the bone was added, false otherwise.
     **/
    bool AddBone( float weight, uint32_t index ) {

        for ( uint32_t i = 0; i < kBoneCountPerControlPoint; ++i ) {
            if ( indices[ i ] == sInvalidIndex ) {
                weights[ i ] = weight;
                indices[ i ] = index;
                return true;
            }
        }

        uint32_t minIndex = 0;
        float minWeight = weights[ 0 ];

        for ( uint32_t i = 1; i < kBoneCountPerControlPoint; ++i ) {
            if ( weights[ i ] < minWeight ) {
                minIndex  = i;
                minWeight = weights[ i ];
            }
        }

        if ( minWeight < weight ) {
            weights[ minIndex ] = weight;
            indices[ minIndex ] = index;
            return true;
        }

        return false;
    }

    /** Prepares assigned bones for GPU skinning.
     * @note Ensures the sum fo the weights is exactly one.
     *       Ensures the indices are valid (vertex packing will get zero or positive bone index).
     **/
    void NormalizeWeights( uint32_t defaultIndex ) {
        for ( uint32_t i = 0; i < kBoneCountPerControlPoint; ++i ) {
            if ( indices[ i ] == sInvalidIndex ) {
                assert( 0.0f == weights[ i ] );
                indices[ i ] = defaultIndex;
            }
        }

        #if 0
        BoneWeightType totalWeight = BoneWeightType( 0 );

        for ( uint32_t i = 0; i < kBoneCountPerControlPoint; ++i ) {
            totalWeight += weights[ i ];
        }

        for ( uint32_t i = 0; i < kBoneCountPerControlPoint; ++i ) {
            weights[ i ] /= totalWeight;
        }
        #endif
    }
};

//
// Flatbuffers takes care about correct platform-independent alignment.
//

static_assert( sizeof( StaticVertex ) == sizeof( apemodefb::StaticVertexFb ), "Must match" );
static_assert( sizeof( StaticSkinnedVertex ) == sizeof( apemodefb::StaticSkinnedVertexFb ), "Must match" );


/**
 * Initialize vertices with very basic properties like 'position', 'normal', 'tangent', 'texCoords'.
 * Calculate mesh position and texcoord min max values.
 **/
template < typename TVertex >
void InitializeVertices( FbxMesh*       mesh,
                         apemode::Mesh& m,
                         TVertex*       vertices,
                         size_t         vertexCount,
                         mathfu::vec3&  positionMin,
                         mathfu::vec3&  positionMax,
                         mathfu::vec2&  texcoordMin,
                         mathfu::vec2&  texcoordMax ) {

    auto& s = apemode::State::Get( );

    const uint32_t cc = (uint32_t) mesh->GetControlPointsCount( );
    const uint32_t pc = (uint32_t) mesh->GetPolygonCount( );

    s.console->info( "Mesh \"{}\" has {} control points.", mesh->GetNode( )->GetName( ), cc );
    s.console->info( "Mesh \"{}\" has {} polygons.", mesh->GetNode( )->GetName( ), pc );

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
    const auto ne  = VerifyElementLayer( mesh->GetElementNormal( ) );
    const auto te  = VerifyElementLayer( mesh->GetElementTangent( ) );

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
            const auto n  = GetElementValue< FbxGeometryElementNormal, FbxVector4 >( ne, ci, vi, pi );
            const auto t  = GetElementValue< FbxGeometryElementTangent, FbxVector4 >( te, ci, vi, pi );

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

    if ( nullptr == te && nullptr != uve ) {
        s.console->warn( "Mesh \"{}\" does not have tangent geometry layer.",
                         mesh->GetNode( )->GetName( ) );

        // Calculate tangents ourselves if UVs are available.
        CalculateTangents( vertices, vertexCount );
    }
}

//
// See implementation in fbxpmeshopt.cpp.
//

void Optimize32( apemode::Mesh& mesh, apemodefb::StaticVertexFb const* vertices, uint32_t & vertexCount, uint32_t vertexStride );
void Optimize16( apemode::Mesh& mesh, apemodefb::StaticVertexFb const* vertices, uint32_t & vertexCount, uint32_t vertexStride );

//
// See implementation in fbxpmeshpacking.cpp.
//

void Pack( const apemodefb::StaticVertexFb* vertices,
           apemodefb::PackedVertexFb*       packed,
           const uint32_t                   vertexCount,
           const mathfu::vec3               positionMin,
           const mathfu::vec3               positionMax,
           const mathfu::vec2               texcoordsMin,
           const mathfu::vec2               texcoordsMax );

uint32_t PackedMaxBoneCount( );

void Pack( const apemodefb::StaticSkinnedVertexFb* vertices,
           apemodefb::PackedSkinnedVertexFb*       packed,
           const uint32_t                          vertexCount,
           const mathfu::vec3                      positionMin,
           const mathfu::vec3                      positionMax,
           const mathfu::vec2                      texcoordsMin,
           const mathfu::vec2                      texcoordsMax );

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

    auto& s = apemode::State::Get( );

    const uint16_t vertexStride                  = (uint16_t) sizeof( apemodefb::StaticVertexFb );
    const uint32_t vertexBufferSize              = vertexCount * vertexStride;
    const uint16_t skinnedVertexStride           = (uint16_t) sizeof( apemodefb::StaticSkinnedVertexFb );
    const uint32_t skinnedVertexBufferSize       = vertexCount * skinnedVertexStride;
    const uint16_t packedVertexStride            = (uint16_t) sizeof( apemodefb::PackedVertexFb );
    const uint32_t packedVertexBufferSize        = vertexCount * packedVertexStride;
    const uint16_t packedSkinnedVertexStride     = (uint16_t) sizeof( apemodefb::PackedSkinnedVertexFb );
    const uint32_t packedSkinnedVertexBufferSize = vertexCount * packedSkinnedVertexStride;

    mathfu::vec3 positionMin;
    mathfu::vec3 positionMax;
    mathfu::vec2 texcoordMin;
    mathfu::vec2 texcoordMax;

    if ( nullptr == pSkin ) {
        m.vertices.resize( vertexBufferSize );
        InitializeVertices( pMesh,
                            m,
                            reinterpret_cast< StaticVertex* >( m.vertices.data( ) ),
                            vertexCount,
                            positionMin,
                            positionMax,
                            texcoordMin,
                            texcoordMax );
    } else {
        m.vertices.resize( skinnedVertexBufferSize );

        /* Currently stick to 4 bones per vertex. */
        using ControlPointSkinInfo = TControlPointSkinInfo<>;
        std::vector< ControlPointSkinInfo > skinInfos;

        /* Allocate skin info for each control point. */
        skinInfos.resize( pMesh->GetControlPointsCount( ) );

        /* Populate skin info for each control point. */
        const int clusterCount = pSkin->GetClusterCount( );
        const bool packTooManyBones = pack && clusterCount > (int) PackedMaxBoneCount( );

        if ( packTooManyBones ) {
            s.console->warn( "Mesh \"{}\" has too large skin, {} bones ({} supported for packing).",
                             pNode->GetName( ),
                             clusterCount,
                             PackedMaxBoneCount( ) );
        }

        m.skinId = (uint32_t) s.skins.size( );

        s.skins.emplace_back( );
        auto& skin = s.skins.back( );
        skin.nameId = s.PushValue( pSkin->GetName( ) );
        skin.linkIds.reserve( clusterCount );

        for ( auto i = 0; i < clusterCount; ++i ) {

            const auto pCluster = pSkin->GetCluster( i );
            assert( nullptr != pCluster );

            const auto indexCount = pCluster->GetControlPointIndicesCount( );
            const auto pWeights = pCluster->GetControlPointWeights( );
            const auto pIndices = pCluster->GetControlPointIndices( );

            if (!indexCount)
                continue;

            assert( 0 != indexCount );
            assert( nullptr != pWeights );
            assert( nullptr != pIndices );

            assert( s.nodeDict.find( pCluster->GetLink( )->GetUniqueID( ) ) != s.nodeDict.end( ) );
            const uint32_t linkNodeId = s.nodeDict[ pCluster->GetLink( )->GetUniqueID( ) ];

            for ( int j = 0; j < indexCount; ++j ) {
                /* Assign bone (weight + index) for the control point. */
                skinInfos[ pIndices[ j ] ].AddBone( (BoneWeightType) pWeights[ j ], i );
                // skinInfos[ pIndices[ j ] ].AddBone( (BoneWeightType) pWeights[ j ], linkNodeId );
            }

            skin.linkIds.push_back( linkNodeId );

            /* TODO: The bind pose matrix must be calculated in the application. */

            /*
            FbxMatrix transformLinkMatrix;
            pCluster->GetTransformLinkMatrix( transformLinkMatrix );
            apemode::SkinLink skinLink;
            skinLink.linkFbxId = pCluster->GetLink( )->GetUniqueID( );
            skinLink.transformLinkMatrix = apemodefb::mat4( apemodefb::vec4( (float) transformLinkMatrix.Get( 0, 0 ),
                                                                             (float) transformLinkMatrix.Get( 0, 1 ),
                                                                             (float) transformLinkMatrix.Get( 0, 2 ),
                                                                             (float) transformLinkMatrix.Get( 0, 3 ) ),
                                                            apemodefb::vec4( (float) transformLinkMatrix.Get( 1, 0 ),
                                                                             (float) transformLinkMatrix.Get( 1, 1 ),
                                                                             (float) transformLinkMatrix.Get( 1, 2 ),
                                                                             (float) transformLinkMatrix.Get( 1, 3 ) ),
                                                            apemodefb::vec4( (float) transformLinkMatrix.Get( 2, 0 ),
                                                                             (float) transformLinkMatrix.Get( 2, 1 ),
                                                                             (float) transformLinkMatrix.Get( 2, 2 ),
                                                                             (float) transformLinkMatrix.Get( 2, 3 ) ),
                                                            apemodefb::vec4( (float) transformLinkMatrix.Get( 3, 0 ),
                                                                             (float) transformLinkMatrix.Get( 3, 1 ),
                                                                             (float) transformLinkMatrix.Get( 3, 2 ),
                                                                             (float) transformLinkMatrix.Get( 3, 3 ) ) );
                                                                             */
        }

        #if 0
        std::set< BoneIndexType > uniqueUsedIndices;

        if ( pack ) {
            for ( auto& skinInfo : skinInfos ) {
                for ( BoneIndexType bi = 0; bi < ControlPointSkinInfo::kBoneCountPerControlPoint; ++bi ) {
                    uniqueUsedIndices.insert( skinInfo.indices[ bi ] );
                }
            }

            auto invalidIt = uniqueUsedIndices.find( sInvalidIndex );
            if ( invalidIt != uniqueUsedIndices.end( ) )
                uniqueUsedIndices.erase( uniqueUsedIndices.find( sInvalidIndex ) );
        }

        /* Remove invalid index to be 100% the bones cannot be reordered. */

        if ( pack ) {

            if ( packTooManyBones && uniqueUsedIndices.size( ) > PackedMaxBoneCount( ) ) {
                s.console->error( "Mesh \"{}\" is influenced by {} bones (only {} \"active\" bones supported).",
                                  pNode->GetName( ),
                                  uniqueUsedIndices.size( ),
                                  PackedMaxBoneCount( ) );

                s.console->warn( "Mesh \"{}\" will exported as static one.", pNode->GetName( ) );

                return ExportMesh< TIndex >( pNode, pMesh, n, m, vertexCount, pack, nullptr, optimize );
            }

            /*

            So what can happen and what I suggest:
            This case handles the situation when we have, for example, 360 bones, but only 200 are influencing this mesh.
            The idea is to minimize the skeleton to 200 bones and change the indices so that it could fit into the 255 range.

            TODO: Find some test cases or compose one yourself and check the solution.

            */

            if ( packTooManyBones && uniqueUsedIndices.size( ) <= PackedMaxBoneCount( ) ) {

                /*

                The code below builds two maps:
                    > original index to node id
                        2 | 3 | 6 | 7 | 9
                        A | B | C | D | E
                    > original index to reordered index
                        2 | 3 | 6 | 7 | 9
                        0 | 1 | 2 | 3 | 4

                The two built maps are used to substitute indices in the skin infos
                and to build a new shorter list of links for exporting.

                */

                std::map< uint16_t, uint64_t > originalIndexToFbxId;
                for ( int boneIndex = 0; boneIndex < clusterCount; ++boneIndex ) {
                    originalIndexToFbxId[ (uint16_t) boneIndex ] = pSkin->GetCluster( boneIndex )->GetLink( )->GetUniqueID( );
                }

                uint32_t reorderedIndexCounter = 0;
                std::map< uint16_t, uint16_t > originalIndexToReorderedIndex;
                for ( auto boneIndex : uniqueUsedIndices ) {
                    originalIndexToReorderedIndex[ boneIndex ] = reorderedIndexCounter++;
                }

                for ( auto& skinInfo : skinInfos ) {
                    for ( BoneIndexType b = 0; b < ControlPointSkinInfo::kBoneCountPerControlPoint; ++b ) {
                        skinInfo.indices[ b ] = originalIndexToReorderedIndex[ skinInfo.indices[ b ] ];
                    }
                }

                skin.linkFbxIds.resize( uniqueUsedIndices.size( ) );

                for ( auto originalIndexReorderedIndex : originalIndexToReorderedIndex ) {
                    skin.linkFbxIds[ originalIndexReorderedIndex.second ] =
                        originalIndexToFbxId[ originalIndexReorderedIndex.first ];
                }
            }
        }
        #endif

        /* Normalize bone weights for each control point. */

        for ( auto& skinInfo : skinInfos ) {
            skinInfo.NormalizeWeights( 0 );
        }

        auto pSkinnedVertices = reinterpret_cast< StaticSkinnedVertex* >( m.vertices.data( ) );
        InitializeVertices( pMesh, m, pSkinnedVertices, vertexCount, positionMin, positionMax, texcoordMin, texcoordMax );

        /* Copy bone weights and indices to each skinned vertex. */

        uint32_t vertexIndex = 0;
        for ( int polygonIndex = 0; polygonIndex < pMesh->GetPolygonCount( ); ++polygonIndex ) {
            for ( const int polygonVertexIndex : {0, 1, 2} ) {

                const int controlPointIndex = pMesh->GetPolygonVertex( polygonIndex, polygonVertexIndex );
                for ( uint32_t b = 0; b < ControlPointSkinInfo::kBoneCountPerControlPoint; ++b ) {
                    assert( skinInfos[ controlPointIndex ].indices[ b ] < skin.linkIds.size( ) );
                    pSkinnedVertices[ vertexIndex ].weights[ b ] = (float) skinInfos[ controlPointIndex ].weights[ b ];
                    pSkinnedVertices[ vertexIndex ].indices[ b ] = (float) skinInfos[ controlPointIndex ].indices[ b ];
                }

                ++vertexIndex;
            }
        }
    }

    GetSubsets( pMesh, m.subsets );
    //GetSubsets< TIndex >( pMesh, m, m.subsets );

    if ( m.subsets.empty( ) ) {
        /* Independently from GetSubsets implementation make sure there is at least one subset. */
        const uint32_t materialId = pMesh->GetNode( )->GetMaterialCount( ) > 0 ? 0 : uint32_t( -1 );
        m.subsets.push_back( apemodefb::SubsetFb( materialId, 0, vertexCount ) );
    }

    /* Fill indices. */

    m.indices.resize( vertexCount * sizeof( TIndex ) );
    TIndex* indices = (TIndex*) m.indices.data( );

    if ( TOrder == EVertexOrder::CCW )
        for ( TIndex i = 0; i < vertexCount; i += 3 ) {
            indices[ i + 0 ] = i + 0;
            indices[ i + 1 ] = i + 2;
            indices[ i + 2 ] = i + 1;
        }
    else
        for ( TIndex i = 0; i < vertexCount; i += 3 ) {
            indices[ i + 0 ] = i + 0;
            indices[ i + 1 ] = i + 1;
            indices[ i + 2 ] = i + 2;
        }

    if ( std::is_same< TIndex, uint16_t >::value ) {
        m.indexType = apemodefb::EIndexTypeFb_UInt16;
    } else if ( std::is_same< TIndex, uint32_t >::value ) {
        m.indexType = apemodefb::EIndexTypeFb_UInt32;
    } else {
        assert( false );
    }

    /*
    if ( optimize ) {
        auto initializedVertices = reinterpret_cast< const apemodefb::StaticVertexFb* >( m.vertices.data( ) );

        if ( std::is_same< TIndex, uint16_t >::value ) {
            Optimize16( m, initializedVertices, vertexCount, vertexStride );
        } else if ( std::is_same< TIndex, uint32_t >::value ) {
            m.subsetIndexType = apemodefb::EIndexTypeFb_UInt32;
            Optimize32( m, initializedVertices, vertexCount, vertexStride );
        }
    }
    */

    if ( pack ) {
        std::vector< uint8_t > vertices( std::move( m.vertices ) );
        if ( nullptr == pSkin ) {
            m.vertices.resize( packedVertexBufferSize );
            Pack( reinterpret_cast< apemodefb::StaticVertexFb* >( vertices.data( ) ),
                  reinterpret_cast< apemodefb::PackedVertexFb* >( m.vertices.data( ) ),
                  vertexCount,
                  positionMin,
                  positionMax,
                  texcoordMin,
                  texcoordMax );
        } else {
            m.vertices.resize( packedSkinnedVertexBufferSize );
            Pack( reinterpret_cast< apemodefb::StaticSkinnedVertexFb* >( vertices.data( ) ),
                  reinterpret_cast< apemodefb::PackedSkinnedVertexFb* >( m.vertices.data( ) ),
                  vertexCount,
                  positionMin,
                  positionMax,
                  texcoordMin,
                  texcoordMax );
        }
    }

    apemodefb::Vec3Fb bboxMin( positionMin.x, positionMin.y, positionMin.z );
    apemodefb::Vec3Fb bboxMax( positionMax.x, positionMax.y, positionMax.z );

    if ( pack ) {
        auto const positionScale = positionMax - positionMin;
        auto const texcoordScale = texcoordMax - texcoordMin;
        apemodefb::Vec2Fb uvMin( texcoordMin.x, texcoordMin.y );
        apemodefb::Vec3Fb const bboxScale( positionScale.x, positionScale.y, positionScale.z );
        apemodefb::Vec2Fb const uvScale( texcoordScale.x, texcoordScale.y );

        const auto submeshVertexStride = nullptr != pSkin ? packedSkinnedVertexStride : packedVertexStride;
        const auto submeshVertexFormat = nullptr != pSkin ? apemodefb::EVertexFormatFb_PackedSkinned : apemodefb::EVertexFormatFb_Packed;

        m.submeshes.emplace_back( bboxMin,                      // bbox min
                                  bboxMax,                      // bbox max
                                  bboxMin,                      // position offset
                                  bboxScale,                    // position scale
                                  uvMin,                        // uv offset
                                  uvScale,                      // uv scale
                                  0,                            // base vertex
                                  vertexCount,                  // vertex count
                                  0,                            // base index
                                  0,                            // index count
                                  0,                            // base subset
                                  (uint32_t) m.subsets.size( ), // subset count
                                  submeshVertexFormat,          // vertex format
                                  submeshVertexStride           // vertex stride
        );
    } else {
        const auto submeshVertexStride = nullptr != pSkin ? skinnedVertexStride : vertexStride;
        const auto submeshVertexFormat = nullptr != pSkin ? apemodefb::EVertexFormatFb_StaticSkinned : apemodefb::EVertexFormatFb_Static;

        m.submeshes.emplace_back( bboxMin,                             // bbox min
                                  bboxMax,                             // bbox max
                                  apemodefb::Vec3Fb( 0.0f, 0.0f, 0.0f ), // position offset
                                  apemodefb::Vec3Fb( 1.0f, 1.0f, 1.0f ), // position scale
                                  apemodefb::Vec2Fb( 0.0f, 0.0f ),       // uv offset
                                  apemodefb::Vec2Fb( 1.0f, 1.0f ),       // uv scale
                                  0,                                   // base vertex
                                  vertexCount,                         // vertex count
                                  0,                                   // base index
                                  0,                                   // index count
                                  0,                                   // base subset
                                  (uint32_t) m.subsets.size( ),        // subset count
                                  submeshVertexFormat,                 // vertex format
                                  submeshVertexStride                  // vertex stride
        );
    }
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
