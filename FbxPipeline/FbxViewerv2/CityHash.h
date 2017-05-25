#pragma once

#include <stdint.h>
#include <functional>

namespace apemode
{
    // Hash function for a byte array.
    unsigned long long CityHash64(const char *buf, unsigned long long len);

    template <bool IsIntegral> struct TCityHash;
    template <> struct TCityHash<false>
    {
        template<typename T>
        static unsigned long long CityHash64(T const & Obj)
        {
            return apemode::CityHash64(
                reinterpret_cast<const char *>(&Obj), 
                sizeof(T)
                );
        }
    };

    template <> struct TCityHash<true>
    {
        template<typename T>
        static unsigned long long CityHash64(T const & Obj)
        {
            return (unsigned long long) Obj;
        }
    };

    // Hash function for a byte array.
    template <typename T, bool IsIntegral = std::is_integral<T>::value> unsigned long long CityHash64(T const & Obj)
    {
        return TCityHash<IsIntegral>::CityHash64(Obj);
    }

    // Hash 128 input bits down to 64 bits of output.
    // This is intended to be a reasonably good hash function.
    unsigned long long CityHash128to64(unsigned long long a, unsigned long long b);

    // Hash 128 input bits down to 64 bits of output.
    // This is intended to be a reasonably good hash function.
    template <typename T, bool IsIntegral = std::is_integral<T>::value> unsigned long long CityHash128to64(unsigned long long a, T const & Obj)
    {
        if (a)
            return CityHash128to64(a, TCityHash<IsIntegral>::CityHash64(Obj));
        else
            return TCityHash<IsIntegral>::CityHash64(Obj);
    }

    // Hash 128 input bits down to 64 bits of output.
    // This is intended to be a reasonably good hash function.
    //unsigned long long Hash128to64(const uint128& x);

    struct CityHash64Wrapper
    {
        typedef uint64_t ValueType;
        typedef std::binary_function<ValueType, ValueType, bool> CmpOpBase;

        ValueType Value;

        CityHash64Wrapper() : Value(0xc949d7c7509e6557ULL) {}
        CityHash64Wrapper(ValueType OtherHash) : Value(OtherHash) {}
        CityHash64Wrapper(CityHash64Wrapper const & Other) : Value(Other.Value) {}

        CityHash64Wrapper(void const * pBuffer, size_t BufferByteSize)
            : Value(CityHash64(reinterpret_cast<char const *>(pBuffer), BufferByteSize))
        {
        }

        template <typename TPlainStructure>
        explicit CityHash64Wrapper(TPlainStructure const & Pod) : Value(CityHash64(Pod))
        {
        }

        void CombineWith(CityHash64Wrapper const & Other)
        {
            Value = CityHash128to64(Value, Other.Value);
        }

        template <typename TPlainStructure>
        void CombineWith(TPlainStructure const & Pod)
        {
            Value = CityHash128to64(Value, Pod);
        }

        void CombineWithBuffer(void const *pBuffer, size_t BufferByteSize)
        {
            auto const pBytes = reinterpret_cast<char const *>(pBuffer);
            auto const BufferHash = CityHash64(pBytes, BufferByteSize);
            Value = CityHash128to64(Value, BufferHash);
        }

        template <typename TPlainStructure>
        void CombineWithArray(TPlainStructure const *pStructs, size_t StructCount)
        {
            static size_t const StructStride = sizeof(TPlainStructure);

            auto const pBytes = reinterpret_cast<char const *>(pStructs);
            auto const ByteCount = StructCount * StructStride;
            auto const BufferHash = CityHash64(pBytes, ByteCount);
            Value = CityHash128to64(Value, BufferHash);
        }

        template <typename TPlainStructure>
        void CombineWithArray(TPlainStructure const *pStructsIt, TPlainStructure const *pStructsItEnd)
        {
            static size_t const StructStride = sizeof(TPlainStructure);

            auto const pBytes = reinterpret_cast<char const *>(pStructsIt);
            auto const ByteCount = static_cast<size_t>(std::distance(pStructsIt, pStructsItEnd));
            auto const BufferHash = CityHash64(pBytes, ByteCount);
            Value = CityHash128to64(Value, BufferHash);
        }

        CityHash64Wrapper & operator<<(CityHash64Wrapper const & Other)
        {
            CombineWith(Other);
            return *this;
        }

        template <typename TPlainStructure>
        CityHash64Wrapper & operator<<(TPlainStructure const & Pod)
        {
            CombineWith(Pod);
            return *this;
        }

        operator uint64_t() const { return Value; }

        struct CmpOpLess : public CmpOpBase
        {
            bool operator()(ValueType HashA, ValueType HashB) const
            {
                return HashA < HashB;
            }
        };
        struct CmpOpGreater : public CmpOpBase
        {
            bool operator()(ValueType HashA, ValueType HashB) const
            {
                return HashA > HashB;
            }
        };
        struct CmpOpGreaterEqual : public CmpOpBase
        {
            bool operator()(ValueType HashA, ValueType HashB) const
            {
                return HashA >= HashB;
            }
        };
        struct CmpOpEqual : public CmpOpBase
        {
            bool operator()(ValueType HashA, ValueType HashB) const
            {
                return HashA == HashB;
            }
        };

        template <typename THashmapKey = size_t>
        struct HashOp : protected std::hash<THashmapKey>
        {
            typedef std::hash<size_t> Base;
            THashmapKey operator()(ValueType const & Hashed) const { return Base::operator()(Hashed); }
        };
    };

}
