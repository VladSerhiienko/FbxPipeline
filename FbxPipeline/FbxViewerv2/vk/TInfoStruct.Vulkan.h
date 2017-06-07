#pragma once

/* This file can be used outside the other framework with
 * TInfoStructHasStructType.Vulkan.inl
 * TInfoStructResolveStructType.Vulkan.inl
 */

namespace apemodevk
{
    /* Utilities to help to resolve the sType member in structs.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    namespace traits
    {
        struct HasInfoStructureTypeResolver {
            template < typename TVulkanStructure >
            struct ResolveFor {
                static const bool bHas = false;
            };

#include <TInfoStructHasStructType.Vulkan.inl>
        };

        struct InfoStructureTypeResolver {
            static const VkStructureType UnresolvedType = VK_STRUCTURE_TYPE_MAX_ENUM;

            template < typename TVulkanStructure >
            static bool IsValid( TVulkanStructure const &Desc ) {
                using HasTypeField = HasInfoStructureTypeResolver::ResolveFor< TVulkanStructure >;

                const auto sType = *reinterpret_cast< VkStructureType const * >( &Desc );
                const bool bMustHaveValidType = HasTypeField::bHas;
                const bool bValid = ( !bMustHaveValidType ) || ( bMustHaveValidType && ( sType != UnresolvedType ) );
                return bValid;
            }

            template < typename TVulkanStructure >
            struct ResolveFor {
                static const VkStructureType eType = UnresolvedType;
            };

#include <TInfoStructResolveStructType.Vulkan.inl>
        };
    }

    /* Utility structure that wraps the native Vulkan structures and sets sType if applicable
     * Since it is quite time consuming to substitute all the usages in the exising code 
     * There are global functions implemented below the class to address the issue
     */
    template <typename TVulkanNativeStruct>
    class TInfoStruct
    {
    public:
        static const bool HasStructType = traits::HasInfoStructureTypeResolver::ResolveFor< TVulkanNativeStruct >::bHas;

        typedef TVulkanNativeStruct VulkanNativeStructType;
        typedef TInfoStruct< TVulkanNativeStruct > SelfType;
        typedef std::vector< SelfType > VectorImpl; /* TODO: Remove */

        /* TODO: Remove */
        struct Vector : public VectorImpl {
            static_assert( sizeof( SelfType ) == sizeof( TVulkanNativeStruct ), "Size mismatch." );
            inline TVulkanNativeStruct *data( ) {
                return reinterpret_cast< TVulkanNativeStruct * >( VectorImpl::data( ) );
            }

            inline TVulkanNativeStruct const *data( ) const {
                return reinterpret_cast< TVulkanNativeStruct const * >( VectorImpl::data( ) );
            }
        };

        TVulkanNativeStruct Desc;

        inline TInfoStruct( ) { InitializeStruct( ); }
        inline TInfoStruct( SelfType &&Other ) : Desc( Other.Desc ) { }
        inline TInfoStruct( SelfType const &Other ) : Desc( Other.Desc ) { }
        inline TInfoStruct( TVulkanNativeStruct const &OtherDesc ) : Desc( OtherDesc ) { SetStructType( ); }

        inline SelfType & operator =(SelfType && Other) { Desc = Other.Desc; Validate(); return *this; }
        inline SelfType & operator =(TVulkanNativeStruct && OtherDesc) { Desc = OtherDesc; Validate(); return *this; }
        inline SelfType & operator =(SelfType const & Other) { Desc = Other.Desc; Validate(); return *this; }
        inline SelfType & operator =(TVulkanNativeStruct const & OtherDesc) { Desc = OtherDesc; Validate();  return *this; }

        inline TVulkanNativeStruct * operator->() { return &Desc; }
        inline TVulkanNativeStruct const * operator->() const { return &Desc; }
        inline operator TVulkanNativeStruct &() { Validate(); return Desc; }
        inline operator TVulkanNativeStruct *() { Validate(); return &Desc; }
        inline operator TVulkanNativeStruct const &() const { Validate(); return Desc; }
        inline operator TVulkanNativeStruct const *() const { Validate(); return &Desc; }

        inline void InitializeStruct( ) {
            InitializeStruct( Desc );
        }

        inline void Validate( ) const {
            Validate( Desc );
        }

        static void Validate( const TVulkanNativeStruct &NativeDesc ) {
            using sTypeResolver = traits::InfoStructureTypeResolver;
            assert( sTypeResolver::IsValid( NativeDesc ) && "Desc is corrupted." );
        }

        static void SetStructType( TVulkanNativeStruct &NativeDesc ) {
            using sTypeResolver = traits::InfoStructureTypeResolver;
            if ( HasStructType ) {
                auto sType = reinterpret_cast< VkStructureType * >( &NativeDesc );
                *sType = sTypeResolver::ResolveFor< TVulkanNativeStruct >::eType;
            }
        }

        template < size_t _ArraySize >
        static void SetStructType( TVulkanNativeStruct ( &NativeDescs )[ _ArraySize ] ) {
            using sTypeResolver = traits::InfoStructureTypeResolver;
            if ( HasStructType ) {
                auto sType = reinterpret_cast< VkStructureType * >( &NativeDescs[ 0 ] );
                for ( auto &NativeDesc : NativeDescs ) {
                    *sType = sTypeResolver::ResolveFor< TVulkanNativeStruct >::eType;
                    sType = reinterpret_cast<VkStructureType*>((reinterpret_cast<uint8_t*>(sType) + sizeof( TVulkanNativeStruct )));
                }
            }
        }

        static void SetStructType( TVulkanNativeStruct * pNativeDescs, size_t pNativeDescCount ) {
            using sTypeResolver = traits::InfoStructureTypeResolver;
            if ( HasStructType ) {
                auto sType = reinterpret_cast< VkStructureType * >( pNativeDescs );
                for ( size_t i = 0; i < pNativeDescCount; ++i ) {
                    *sType = sTypeResolver::ResolveFor< TVulkanNativeStruct >::eType;
                    sType += sizeof( TVulkanNativeStruct );
                }
            }
        }

        static void InitializeStruct( TVulkanNativeStruct &Desc ) {
            ZeroMemory( Desc );
            SetStructType( Desc );
        }

        template < size_t _ArraySize >
        static void InitializeStruct( TVulkanNativeStruct ( &NativeDescs )[ _ArraySize ] ) {
            ZeroMemory( NativeDescs );
            SetStructType( NativeDescs );
        }

        static void InitializeStruct( TVulkanNativeStruct *pNativeDescs, size_t pNativeDescCount ) {
            ZeroMemory( pNativeDescs, pNativeDescCount );
            SetStructType( pNativeDescs, pNativeDescCount );
        }
    };

    ///
    /// Utility structures, that make it easier to integrate
    /// the Vulkan wrapper into the existing code bases.
    ///

    /* Sets sType structure member if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct >
    inline void SetStructType( TVulkanNativeStruct &NativeDesc ) {
        TInfoStruct< TVulkanNativeStruct >::SetStructType( NativeDesc );
    }

    /* Sets sType structure members if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct, size_t _ArraySize >
    inline void SetStructType( TVulkanNativeStruct ( &NativeDescs )[ _ArraySize ] ) {
        TInfoStruct< TVulkanNativeStruct >::SetStructType( NativeDescs );
    }

    /* Sets sType structure members if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct >
    inline void SetStructType( TVulkanNativeStruct *pNativeDescs, size_t pNativeDescCount ) {
        TInfoStruct< TVulkanNativeStruct >::SetStructType( pNativeDescs, pNativeDescCount );
    }

    /* Zeros all the provided memory, and sets sType structure members if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct >
    inline void InitializeStruct( TVulkanNativeStruct &NativeDesc ) {
        TInfoStruct< TVulkanNativeStruct >::InitializeStruct( NativeDesc );
    }

    /* Zeros all the provided memory, and sets sType structure members if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct, size_t _ArraySize >
    inline void InitializeStruct( TVulkanNativeStruct ( &NativeDescs )[ _ArraySize ] ) {
        TInfoStruct< TVulkanNativeStruct >::InitializeStruct( NativeDescs );
    }

    /* Zeros all the provided memory, and sets sType structure members if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct >
    inline void InitializeStruct( TVulkanNativeStruct *pNativeDescs, size_t pNativeDescCount ) {
        TInfoStruct< TVulkanNativeStruct >::InitializeStruct( pNativeDescs, pNativeDescCount );
    }

    /* Please, see InitializeStruct( ) */
    template < typename TVulkanNativeStruct >
    TVulkanNativeStruct NewInitializedStruct( ) {
        TVulkanNativeStruct NativeDesc;
        InitializeStruct( NativeDesc );
        return NativeDesc;
    }

} // namespace apemodevk

namespace apemodevk
{
    template <typename T>
    inline uint32_t GetSizeU(T const & c) {
        return _Get_collection_length_u(c);
    }

    /** Aliasing cares only about size matching. */
    template < typename TVector, typename TNativeDesc >
    static void AliasStructs( TVector const &Descs, TNativeDesc const *&pOutDescs, uint32_t &OutDescCount ) {
        using ValueType = TVector::value_type;
        static_assert( sizeof( ValueType ) == sizeof( TNativeDesc ), "Size mismatch, cannot alias." );
        pOutDescs = reinterpret_cast< TNativeDesc const * >( Descs.data( ) );
        OutDescCount = GetSizeU( Descs );
    }

    /** Aliasing cares both about type and size matching.
     * @see AliasStructs for the lighter version of the function.
     */
    template < typename TInfoStructVector, typename TNativeDesc >
    static void AliasInfoStructs( TInfoStructVector const &Descs,
                                  TNativeDesc const *&     pOutAliasedDescsRef,
                                  uint32_t &               OutDescCount ) {
        using InfoStructValueType = TInfoStructVector::value_type;
        using NativeDescValueType = InfoStructValueType::VulkanNativeStructType;
        static_assert( std::is_same< TNativeDesc, NativeDescValueType >::value, "Types are not same, cannot alias." );
        AliasStructs( Descs, pOutAliasedDescsRef, OutDescCount );
    }
} // namespace apemodevk
