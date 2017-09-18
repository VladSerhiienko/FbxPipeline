#pragma once
#include <fbxppch.h>

/**
 * Packing utilities.
 **/

namespace apemode {

    template < int BitCount >
    struct ChooseUint {
        typedef uint64_t type;
    };

    template <>
    struct ChooseUint< 2 > {
        typedef uint8_t type;
    };

    template <>
    struct ChooseUint< 4 > {
        typedef uint8_t type;
    };

    template <>
    struct ChooseUint< 8 > {
        typedef uint8_t type;
    };

    template <>
    struct ChooseUint< 10 > {
        typedef uint16_t type;
    };

    template <>
    struct ChooseUint< 11 > {
        typedef uint16_t type;
    };

    template <>
    struct ChooseUint< 16 > {
        typedef uint16_t type;
    };

    template <>
    struct ChooseUint< 24 > {
        typedef uint32_t type;
    };

    template <>
    struct ChooseUint< 32 > {
        typedef uint32_t type;
    };

    template <>
    struct ChooseUint< 64 > {
        typedef uint64_t type;
    };

    namespace details {
        inline float clamp( float val, float low, float hi ) {
            if ( val <= low ) {
                return low;
            } else if ( val >= hi ) {
                return hi;
            } else {
                return val;
            }
        }

        inline float round( float f ) {
            return floor( f + 0.5f );
        }
    }

    template < int BitCount >
    class Snorm {
    private:
        typedef typename ChooseUint< BitCount >::type T;
        static const T sMax = T( 1 ) << ( BitCount - 1 );

        T m_bits;

        /**
         * Private to prevent illogical conversions without explicitly
         * stating that the input should be treated as bits; @see FromBits.
         **/
        explicit Snorm( T b ) : m_bits( b ) {
        }

    public:
        explicit Snorm( ) : m_bits( 0 ) {
        }

        /**
         * Equivalent to:
         * @code Snorm8 u = reinterpret_cast<const Snorm8&>(255);@endcode
         **/
        static Snorm FromBits( T b ) {
            return Snorm( b );
        }

        /** Maps f to the underlying bits.*/
        explicit Snorm( float f ) {
            m_bits = (T) details::round( details::clamp( f, -1.0f, 1.0f ) * ( sMax - 1 ) + sMax );
        }

        /**
         * Finds the top left corner of the corresponding square to allow for discrete searching.
         **/
        static Snorm FlooredSnorms( float f ) {
            return FromBits( ( T )( floor( details::clamp( f, -1.0f, 1.0f ) * ( sMax - 1 ) + sMax ) ) );
        }

        /**
         * Returns a number on [0.0f, 1.0f].
         **/
        operator float( ) const {
            return float( details::clamp( ( float( m_bits ) - sMax ) * ( 1.0f / float( sMax - 1 ) ), -1.0f, 1.0f ) );
        }

        /**
         * Returns packed int value.
         **/
        T Bits( ) const {
            return m_bits;
        }
    };

    template < int BitCount >
    class Unorm {
    public:
        typedef typename ChooseUint< BitCount >::type T;
        static const T sMax = ( T( 1 ) << BitCount ) - 1;

    private:
        T m_bits;

        /**
         * Private to prevent illogical conversions without explicitly
         * stating that the input should be treated as bits; @see FromBits.
         **/
        explicit Unorm( T b ) : m_bits( b ) {
        }

    public:
        explicit Unorm( ) : m_bits( 0 ) {
        }

        /**
         * Equivalent to:
         * @code Unorm8 u = reinterpret_cast<const Unorm8&>(255);@endcode
         **/
        static Unorm FromBits( T b ) {
            return Unorm( b );
        }

        /**
         * Maps f to the underlying bits.
         **/
        explicit Unorm( float f ) {
            m_bits = (T) details::round( details::clamp( f, 0.0f, 1.0f ) * sMax );
        }

        /**
         * Finds the top left corner of the corresponding square to allow for discrete searching.
         **/
        static Unorm FlooredUnorms( float f ) {
            return FromBits( ( T )( floor( details::clamp( f, 0.0f, 1.0f ) * sMax ) ) );
        }

        /**
         * Returns a number on [0.0f, 1.0f].
         **/
        operator float( ) const {
            return float( details::clamp( int( m_bits ) * ( 1.0f / float( sMax ) ), 0.0f, 1.0f ) );
        }

        /**
         * Returns packed int value.
         **/
        T Bits( ) const {
            return m_bits;
        }
    };

    /**
     * IEEE float 16
     * Represented by 10-bit mantissa M, 5-bit exponent E, and 1-bit sign S
     **/
    template < bool bOverflowCheck = true >
    class Half {
    private:
        union Float16Pack {
            struct {
                uint16_t m : 10;
                uint16_t e : 5;
                uint16_t s : 1;
            } q;

            uint16_t u;
        };

        union Float32Pack {
            struct {
                uint32_t m : 23;
                uint32_t e : 8;
                uint32_t s : 1;
            } q;

            float_t  f;
            uint32_t u;
        };

    private:
        uint16_t m_bits;

        /**
         * Private to prevent illogical conversions without explicitly
         * stating that the input should be treated as bits; @see FromBits.
         **/
        explicit Half( uint16_t b ) : m_bits( b ) {
        }

    public:
        explicit Half( ) : m_bits( 0 ) {
        }

        /**
         * Equivalent to:
         * @code Half u = reinterpret_cast<const Half&>(255);@endcode
         **/
        static Half FromBits( uint16_t b ) {
            return Half( b );
        }

        /**
         * Maps f to the underlying bits.
         **/
        explicit Half( float v ) {
            Float32Pack f;
            Float16Pack h;

            f.f   = v;
            h.q.s = f.q.s;

            if ( bOverflowCheck && f.q.e <= 112 ) {
                // Check for zero, denormal or too small value.
                // Too small exponent? (0+127-15)
                // Set to 0.
                h.q.e = 0;
                h.q.m = 0;
                // DebugBreak( );
            } else if ( bOverflowCheck && f.q.e >= 143 ) {
                // Check for INF or NaN, or too high value
                // Too large exponent? (31+127-15)
                // Set to 65504.0 (max value)
                h.q.e = 30;
                h.q.m = 1023;
                // DebugBreak( );
            } else {
                // Handle normal number.
                h.q.e = int32_t( f.q.e ) - 127 + 15;
                h.q.m = uint16_t( f.q.m >> 13 );
            }

            m_bits = h.u;
        }

        /**
         * Returns a mapped float.
         **/
        operator float( ) const {
            Float32Pack f;
            Float16Pack h;

            h.u = m_bits;
            f.q.s = h.q.s;

            if ( h.q.e == 0 ) {
                uint32_t Mantissa = h.q.m;
                if ( Mantissa == 0 ) {
                    // Zero.
                    f.q.Exponent = 0;
                    f.q.Mantissa = 0;
                } else {
                    // Denormal.
                    uint32_t MantissaShift = 10 - (uint32) FMath::TruncToInt( FMath::Log2( Mantissa ) );
                    f.q.e = 127 - ( 15 - 1 ) - MantissaShift;
                    f.q.m = Mantissa << ( MantissaShift + 23 - 10 );
                }
            } else if ( h.q.e == 31 ) {
                // Infinity or NaN. Set to 65504.0
                f.q.e = 142;
                f.q.m = 8380416;
                // DebugBreak( );
            } else {
                // Normal number.
                // Stored exponents are biased by half their range.
                f.q.e = int32_t( h.q.e ) - 15 + 127;
                f.q.m = uint32_t( h.q.m ) << 13;
            }

            return f.f;
        }

        /**
         * Returns packed int value.
         **/
        uint16_t Bits( ) const {
            return m_bits;
        }
    };
}

