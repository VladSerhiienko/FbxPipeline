#pragma once

#include <fbxvpch.h>

namespace fbxv {
    void *Malloc( size_t bytes );
    void *Memalign( size_t alignment, size_t bytes );
    void Free( void *p );

    namespace bgfxUtils {
        void releaseRef( void *data, void *userData );
        const bgfx::Memory *makeReleasableCopy( const void *data, size_t dataSize );
        const bgfx::Memory *makeReleasableRef( const void *data, size_t dataSize );
    }
}

namespace fbxv {

    struct StaticVertex {
        static bgfx::VertexDecl vertexDecl;
        static void InitializeOnce( ) {
            static bool sInitialized = false;
            if ( !sInitialized ) {
                sInitialized = true;
                vertexDecl.begin( )
                    .add( bgfx::Attrib::Position, 3, bgfx::AttribType::Float, false, false )
                    .add( bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, false, false )
                    .add( bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float, false, false )
                    .add( bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float, false, false )
                    .end( );
            }
        }
    };

    struct PackedVertex {
        static bgfx::VertexDecl vertexDecl;
        static void InitializeOnce( ) {
            static bool sInitialized = false;
            if ( !sInitialized ) {
                sInitialized = true;
                /*vertexDecl.begin( )
                    .add( bgfx::Attrib::Position, 3, bgfx::AttribType::Float, false, false )
                    .add( bgfx::Attrib::Normal, 3, bgfx::AttribType::Uint10, true, true )
                    .add( bgfx::Attrib::Tangent, 4, bgfx::AttribType::Uint10, true, true )
                    .add( bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Half, false, false )
                    .end( );*/
                vertexDecl.begin( )
                    .add( bgfx::Attrib::Position, 3, bgfx::AttribType::Float, false, false )
                    .add( bgfx::Attrib::Normal, 3, bgfx::AttribType::Uint8, true, false )
                    .add( bgfx::Attrib::Tangent, 4, bgfx::AttribType::Uint8, true, false )
                    .add( bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Half, false, false )
                    .end( );
            }
        }
    };

    bgfx::VertexDecl StaticVertex::vertexDecl;
    bgfx::VertexDecl PackedVertex::vertexDecl;

    struct SceneMaterial {
        mathfu::vec4 albedo;
    };

    struct SceneMeshSubset {
        uint32_t materialId = -1;
        uint32_t baseIndex  = 0;
        uint32_t indexCount = 0;
    };

    struct SceneMesh {
        bgfx::VertexBufferHandle       vertexBufferHandle;
        bgfx::IndexBufferHandle        indexBufferHandle;
        std::vector< SceneMeshSubset > subsets;
    };

    /**
     * Transfrom class that stores main FBX SDK transform properties
     * and calculates local and geometric matrices.
     **/
    struct SceneNodeTransform {
        mathfu::vec3 translation;
        mathfu::vec3 rotationOffset;
        mathfu::vec3 rotationPivot;
        mathfu::vec3 preRotation;
        mathfu::vec3 postRotation;
        mathfu::vec3 rotation;
        mathfu::vec3 scalingOffset;
        mathfu::vec3 scalingPivot;
        mathfu::vec3 scaling;
        mathfu::vec3 geometricTranslation;
        mathfu::vec3 geometricRotation;
        mathfu::vec3 geometricScaling;

        /**
         * Checks for nans and zero scales.
         * @return True if valid, false otherwise.
         **/
        bool Validate() const {
            return false == ( isnan( translation.x( ) ) || isnan( translation.y( ) ) || isnan( translation.z( ) ) ||
                              isnan( rotationOffset.x( ) ) || isnan( rotationOffset.y( ) ) || isnan( rotationOffset.z( ) ) ||
                              isnan( rotationPivot.x( ) ) || isnan( rotationPivot.y( ) ) || isnan( rotationPivot.z( ) ) ||
                              isnan( preRotation.x( ) ) || isnan( preRotation.y( ) ) || isnan( preRotation.z( ) ) ||
                              isnan( postRotation.x( ) ) || isnan( postRotation.y( ) ) || isnan( postRotation.z( ) ) ||
                              isnan( rotation.x( ) ) || isnan( rotation.y( ) ) || isnan( rotation.z( ) ) ||
                              isnan( scalingOffset.x( ) ) || isnan( scalingOffset.y( ) ) || isnan( scalingOffset.z( ) ) ||
                              isnan( scalingPivot.x( ) ) || isnan( scalingPivot.y( ) ) || isnan( scalingPivot.z( ) ) ||
                              isnan( scaling.x( ) ) || isnan( scaling.y( ) ) || isnan( scaling.z( ) ) ||
                              isnan( geometricTranslation.x( ) ) || isnan( geometricTranslation.y( ) ) ||
                              isnan( geometricTranslation.z( ) ) || isnan( geometricRotation.x( ) ) ||
                              isnan( geometricRotation.y( ) ) || isnan( geometricRotation.z( ) ) ||
                              isnan( geometricScaling.x( ) ) || isnan( geometricScaling.y( ) ) ||
                              isnan( geometricScaling.z( ) ) || scaling.x( ) == 0 || scaling.y( ) == 0 || scaling.z( ) == 0 ||
                              geometricScaling.x( ) == 0 || geometricScaling.y( ) == 0 || geometricScaling.z( ) == 0 );
        }

        /**
         * Calculates local matrix.
         * @note Contributes node world matrix.
         * @return Node local matrix.
         **/
        inline mathfu::mat4 CalculateLocalMatrix( ) const {
            return mathfu::mat4::FromTranslationVector( translation ) *
                   mathfu::mat4::FromTranslationVector( rotationOffset ) *
                   mathfu::mat4::FromTranslationVector( rotationPivot ) *
                   mathfu::quat::FromEulerAngles( preRotation ).ToMatrix4( ) *
                   mathfu::quat::FromEulerAngles( rotation ).ToMatrix4( ) *
                   mathfu::quat::FromEulerAngles( postRotation ).ToMatrix4( ) *
                   mathfu::mat4::FromTranslationVector( -rotationPivot ) *
                   mathfu::mat4::FromTranslationVector( scalingOffset ) *
                   mathfu::mat4::FromTranslationVector( scalingPivot ) *
                   mathfu::mat4::FromScaleVector( scaling ) *
                   mathfu::mat4::FromTranslationVector( -scalingPivot );
        }

        /**
         * Calculate geometric transform.
         * @note That is an object-offset 3ds Max concept,
         *       it does not influence scene hierarchy, though
         *       it contributes to node world transform.
         * @return Node geometric transform.
         **/
        inline mathfu::mat4 CalculateGeometricMatrix( ) const {
            return mathfu::mat4::FromTranslationVector( geometricTranslation ) *
                   mathfu::quat::FromEulerAngles( geometricRotation ).ToMatrix4( ) *
                   mathfu::mat4::FromScaleVector( geometricScaling );
        }
    };

    struct Scene;
    struct SceneNode {
        Scene*                  scene    = nullptr;
        uint32_t                id       = -1;
        uint32_t                parentId = -1;
        uint32_t                meshId   = -1;
        std::vector< uint32_t > childIds;
        std::vector< uint32_t > materialIds;
    };

    struct Scene {
        //
        // Scene components
        //

        std::vector< SceneNode >          nodes;
        std::vector< SceneNodeTransform > transforms;
        std::vector< SceneMesh >          meshes;
        std::vector< SceneMaterial >      materials;

        //
        // Transform matrices storage.
        //

        std::vector< mathfu::mat4 > worldMatrices;
        std::vector< mathfu::mat4 > localMatrices;
        std::vector< mathfu::mat4 > geometricMatrices;
        std::vector< mathfu::mat4 > hierarchicalMatrices;

        /**
         * Internal usage only.
         * @see UpdateMatrices().
         * @todo Safety assertions.
         **/
        inline void UpdateChildWorldMatrices( uint32_t nodeId ) {
            for ( auto& childId : nodes[ nodeId ].childIds ) {

                localMatrices[ childId ]        = transforms[ childId ].CalculateLocalMatrix( );
                geometricMatrices[ childId ]    = transforms[ childId ].CalculateGeometricMatrix( );
                hierarchicalMatrices[ childId ] = hierarchicalMatrices[ nodes[ childId ].parentId ] * localMatrices[ childId ];
                worldMatrices[ childId ]        = hierarchicalMatrices[ childId ] * geometricMatrices[ childId ];

                if ( false == nodes[ childId ].childIds.empty( ) )
                    UpdateChildWorldMatrices( childId );
            }
        }

        /**
         * Update matrices storage with up-to-date values.
         * @todo Non-recursive, no dynamic memory?
         *       Currently, I tried to avoid any stack memory pollution,
         *       tried to avoid any local variable but kept it clean.
         **/
        inline void UpdateMatrices() {
            if ( transforms.empty( ) || nodes.empty( ) )
                return;

            //
            // Resize matrices storage if needed
            //

            if ( localMatrices.size( ) < transforms.size( ) ) {
                localMatrices.resize( transforms.size( ) );
                worldMatrices.resize( transforms.size( ) );
                geometricMatrices.resize( transforms.size( ) );
                hierarchicalMatrices.resize( transforms.size( ) );
            }

            //
            // Implicit world calculations for the root node.
            //

            localMatrices[ 0 ]        = transforms[ 0 ].CalculateLocalMatrix( );
            geometricMatrices[ 0 ]    = transforms[ 0 ].CalculateGeometricMatrix( );
            hierarchicalMatrices[ 0 ] = localMatrices[ 0 ];
            worldMatrices[ 0 ]        = localMatrices[ 0 ] * geometricMatrices[ 0 ];

            //
            // Start recursive updates from root node.
            //

            UpdateChildWorldMatrices( 0 );
        }

        template < typename TNodeIdCallback >
        void ForEachNode( uint32_t nodeId, TNodeIdCallback callback ) {
            callback( nodeId );
            for ( auto &childId : nodes[ nodeId ].childIds ) {
                ForEachNode( childId, callback );
            }
        }

        template < typename TNodeIdCallback >
        void ForEachNode( TNodeIdCallback callback ) {
            ForEachNode( 0, callback );
        }
    };

    Scene * LoadSceneFromFile(const char * filename) {
        std::string fileData;
        if ( flatbuffers::LoadFile( filename, true, &fileData ) ) {
            if ( auto sceneFb = fbxp::fb::GetSceneFb( fileData.c_str( ) ) ) {
                std::unique_ptr< Scene > scene( new Scene( ) );

                //
                // Since the format is not final, I do not rely much on a
                // current structure and do not use memcpy, I want to keep
                // the viewer stable to scene generated header changes.
                //

                if ( auto nodesFb = sceneFb->nodes( ) ) {
                    scene->nodes.resize( nodesFb->size( ) );
                    scene->transforms.resize( nodesFb->size( ) );

                    const float toRadsFactor = M_PI / 180;

                    for ( auto nodeFb : *nodesFb ) {
                        assert( nodeFb );

                        auto& node = scene->nodes[ nodeFb->id( ) ];

                        node.id     = nodeFb->id( );
                        node.scene  = scene.get( );
                        node.meshId = nodeFb->mesh_id( );

                        if ( nodeFb->child_ids( ) && nodeFb->child_ids( )->size( ) )
                            node.childIds.reserve( nodeFb->child_ids( )->size( ) );

                        if ( nodeFb->material_ids( ) && nodeFb->material_ids( )->size( ) )
                            node.materialIds.reserve( nodeFb->material_ids( )->size( ) );

                        //
                        // Scene does not store parent node ids.
                        // We can set them with no additional effort or data at this stage.
                        //

                        auto childIdsIt = nodeFb->child_ids( )->data( );
                        auto childIdsEndIt = childIdsIt + nodeFb->child_ids( )->size( );
                        std::transform( childIdsIt, childIdsEndIt, std::back_inserter( node.childIds ), [&]( auto id ) {
                            auto& childNode = scene->nodes[ id ];
                            childNode.parentId = node.id;
                            return id;
                        } );

                        if (nodeFb->material_ids() && nodeFb->material_ids()->size()) {
                            auto matIdsIt    = nodeFb->material_ids( )->data( );
                            auto matIdsEndIt = matIdsIt + nodeFb->material_ids( )->size( );
                            std::transform( matIdsIt, matIdsEndIt, std::back_inserter( node.materialIds ), [&]( auto id ) { return id; } );
                        }

                        auto& transform   = scene->transforms[ nodeFb->id( ) ];
                        auto  transformFb = ( *sceneFb->transforms( ) )[ nodeFb->id( ) ];

                        transform.translation.x( )          = transformFb->translation( ).x( );
                        transform.translation.y( )          = transformFb->translation( ).y( );
                        transform.translation.z( )          = transformFb->translation( ).z( );
                        transform.rotationOffset.x( )       = transformFb->rotation_offset( ).x( );
                        transform.rotationOffset.y( )       = transformFb->rotation_offset( ).y( );
                        transform.rotationOffset.z( )       = transformFb->rotation_offset( ).z( );
                        transform.rotationPivot.x( )        = transformFb->rotation_pivot( ).x( );
                        transform.rotationPivot.y( )        = transformFb->rotation_pivot( ).y( );
                        transform.rotationPivot.z( )        = transformFb->rotation_pivot( ).z( );
                        transform.preRotation.x( )          = transformFb->pre_rotation( ).x( ) * toRadsFactor;
                        transform.preRotation.y( )          = transformFb->pre_rotation( ).y( ) * toRadsFactor;
                        transform.preRotation.z( )          = transformFb->pre_rotation( ).z( ) * toRadsFactor;
                        transform.rotation.x( )             = transformFb->rotation( ).x( ) * toRadsFactor;
                        transform.rotation.y( )             = transformFb->rotation( ).y( ) * toRadsFactor;
                        transform.rotation.z( )             = transformFb->rotation( ).z( ) * toRadsFactor;
                        transform.postRotation.x( )         = transformFb->post_rotation( ).x( ) * toRadsFactor;
                        transform.postRotation.y( )         = transformFb->post_rotation( ).y( ) * toRadsFactor;
                        transform.postRotation.z( )         = transformFb->post_rotation( ).z( ) * toRadsFactor;
                        transform.scalingOffset.x( )        = transformFb->scaling_offset( ).x( );
                        transform.scalingOffset.y( )        = transformFb->scaling_offset( ).y( );
                        transform.scalingOffset.z( )        = transformFb->scaling_offset( ).z( );
                        transform.scalingPivot.x( )         = transformFb->scaling_pivot( ).x( );
                        transform.scalingPivot.y( )         = transformFb->scaling_pivot( ).y( );
                        transform.scalingPivot.z( )         = transformFb->scaling_pivot( ).z( );
                        transform.scaling.x( )              = transformFb->scaling( ).x( );
                        transform.scaling.y( )              = transformFb->scaling( ).y( );
                        transform.scaling.z( )              = transformFb->scaling( ).z( );
                        transform.geometricTranslation.x( ) = transformFb->geometric_translation( ).x( );
                        transform.geometricTranslation.y( ) = transformFb->geometric_translation( ).y( );
                        transform.geometricTranslation.z( ) = transformFb->geometric_translation( ).z( );
                        transform.geometricRotation.x( )    = transformFb->geometric_rotation( ).x( ) * toRadsFactor;
                        transform.geometricRotation.y( )    = transformFb->geometric_rotation( ).y( ) * toRadsFactor;
                        transform.geometricRotation.z( )    = transformFb->geometric_rotation( ).z( ) * toRadsFactor;
                        transform.geometricScaling.x( )     = transformFb->geometric_scaling( ).x( );
                        transform.geometricScaling.y( )     = transformFb->geometric_scaling( ).y( );
                        transform.geometricScaling.z( )     = transformFb->geometric_scaling( ).z( );

                        assert( transform.Validate( ) );
                    }

                    scene->UpdateMatrices( );
                }

                if ( auto meshesFb = sceneFb->meshes( ) ) {
                    PackedVertex::InitializeOnce( );
                    scene->meshes.reserve( meshesFb->size( ) );

                    for ( auto meshFb : *meshesFb ) {
                        assert( meshFb );
                        assert( meshFb->vertices( ) && meshFb->vertices( )->size( ) );
                        assert( meshFb->submeshes( ) && meshFb->submeshes( )->size( ) == 1 );

                        scene->meshes.emplace_back( );
                        auto &mesh = scene->meshes.back( );

                        mesh.vertexBufferHandle = bgfx::createVertexBuffer(
                            bgfxUtils::makeReleasableCopy( meshFb->vertices( )->Data( ), meshFb->vertices( )->size( ) ),
                            PackedVertex::vertexDecl );

                        if ( meshFb->subsets( ) && meshFb->subsets( )->size( ) && meshFb->subset_indices( ) ) {
                            mesh.indexBufferHandle = bgfx::createIndexBuffer(
                                bgfxUtils::makeReleasableCopy( meshFb->subset_indices( )->Data( ),
                                                               meshFb->subset_indices( )->size( ) ),
                                meshFb->subset_index_type( ) == fbxp::fb::EIndexTypeFb_UInt32 ? BGFX_BUFFER_INDEX32 : 0 );
                        }

                        mesh.subsets.reserve( meshFb->subsets( )->size( ) );

                        auto subsetIt    = (const fbxp::fb::SubsetFb *) meshFb->subsets( )->Data( );
                        auto subsetEndIt = subsetIt + meshFb->subsets( )->size( );
                        std::transform( subsetIt,
                                        subsetEndIt,
                                        std::back_inserter( mesh.subsets ),
                                        [&]( const fbxp::fb::SubsetFb &subsetFb ) {
                                            assert( subsetFb.index_count( ) );

                                            SceneMeshSubset subset;
                                            subset.materialId = subsetFb.material_id( );
                                            subset.baseIndex  = subsetFb.base_index( );
                                            subset.indexCount = subsetFb.index_count( );

                                            return subset;
                                        } );
                    }
                }

                if (auto materialsFb = sceneFb->materials()) {
                    scene->materials.reserve( materialsFb->size( ) );

                    for ( auto materialFb : *materialsFb ) {
                        scene->materials.emplace_back( );
                        auto &material = scene->materials.back( );

                        material.albedo.x( ) = mathfu::RandomInRange< float >( 0, 1 );
                        material.albedo.y( ) = mathfu::RandomInRange< float >( 0, 1 );
                        material.albedo.z( ) = mathfu::RandomInRange< float >( 0, 1 );
                        material.albedo.w( ) = 1;
                    }
                }

                return scene.release( );
            }
        }

        return nullptr;
    }

}