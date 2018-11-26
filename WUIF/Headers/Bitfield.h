/*Copyright (c) 2018 Jonathan Campbell

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/

//this was inspired from http://www.artima.com/cppsource/safelabels.html
#pragma once
#include <assert.h>
#ifdef _BITFIELD
//used to create a unique id for the template
struct bitfield_unique_id {};

//forward declaration of checked_bit_mask
//word_t type of "word" to store the bits - should be an integer type
//unique_id - what makes the bit fields into unique types
template <bitfield_unique_id* unique_id, typename word_t>
class checked_bit_mask;

//class declaration
template <bitfield_unique_id* unique_id, typename word_t>
class checked_bit_field
{
private:
    //the actual field.
    word_t word;

    //private constructor from an integer type.
    explicit checked_bit_field(word_t init) noexcept : word(init) {}

public:
    //For convenience with macros, we declare checked_bit_field::fieldbit_t
    friend class checked_bit_mask<unique_id, word_t>;
    typedef checked_bit_mask<unique_id, word_t> fieldbit_t;

    //--Constructors--
    //default constructor - our "word" is zeroed
    checked_bit_field() noexcept : word(static_cast<word_t>(0)) {}
    //copy constructor from bitfield
    checked_bit_field(const checked_bit_field& rhs) noexcept : word(rhs.word) {}
    checked_bit_field(const volatile checked_bit_field& rhs) noexcept : word(rhs.word) {}
    //copy constructor from bit mask
    checked_bit_field(const fieldbit_t& rhs) noexcept : word(rhs.word) {}
    //copy constructor from 0//NULL/nullptr
    checked_bit_field(const std::nullptr_t)  noexcept : word(word_t{ 0 }) {}

    //--Copy Assignments--
    //copy assignment operator
    #ifdef _MSC_VER
    //If you define or =delete any default operation, define or =delete them all
    #pragma warning(suppress : 26432)
    checked_bit_field& operator=(const checked_bit_field& rhs) noexcept { if (this != &rhs) word = rhs.word; return *this; }
    #pragma warning(suppress : 26432)
    #endif
    volatile checked_bit_field& operator=(const volatile checked_bit_field& rhs) volatile noexcept { if (this != &rhs) word = rhs.word; return *this; }
    volatile checked_bit_field& operator=(const checked_bit_field& rhs) volatile noexcept { if (this != &rhs) word = rhs.word; return *this; }
    //copy assignment operator from bit mask
    checked_bit_field& operator=(const fieldbit_t& rhs) noexcept { word = rhs.word; return *this; }
    volatile checked_bit_field& operator=(const fieldbit_t& rhs) volatile noexcept { word = rhs.word; return *this; }
    //assignment operator to allow for assigning 0 (clearing bits)
    checked_bit_field& operator=(const std::nullptr_t&) noexcept { word = 0; return *this; }
    volatile checked_bit_field& operator=(const std::nullptr_t&) volatile noexcept { word = 0; return *this; }

    //--Operations--
    //0/NULL/nullptr to bitfield comparison operators
    friend bool operator==(const std::nullptr_t, const checked_bit_field& rhs) noexcept { return 0 == rhs.word; }
    friend bool operator!=(const std::nullptr_t, const checked_bit_field& rhs) noexcept { return 0 != rhs.word; }
    friend bool operator<=(const std::nullptr_t, const checked_bit_field& rhs) noexcept { return 0 <= rhs.word; }
    friend bool operator>=(const std::nullptr_t, const checked_bit_field& rhs) noexcept { return 0 >= rhs.word; }
    friend bool operator< (const std::nullptr_t, const checked_bit_field& rhs) noexcept { return 0 <  rhs.word; }
    friend bool operator> (const std::nullptr_t, const checked_bit_field& rhs) noexcept { return 0 >  rhs.word; }
    friend bool operator==(const std::nullptr_t, const volatile checked_bit_field& rhs) noexcept { return 0 == rhs.word; }
    friend bool operator!=(const std::nullptr_t, const volatile checked_bit_field& rhs) noexcept { return 0 != rhs.word; }
    friend bool operator<=(const std::nullptr_t, const volatile checked_bit_field& rhs) noexcept { return 0 <= rhs.word; }
    friend bool operator>=(const std::nullptr_t, const volatile checked_bit_field& rhs) noexcept { return 0 >= rhs.word; }
    friend bool operator< (const std::nullptr_t, const volatile checked_bit_field& rhs) noexcept { return 0 <  rhs.word; }
    friend bool operator> (const std::nullptr_t, const volatile checked_bit_field& rhs) noexcept { return 0 >  rhs.word; }
    //0/NULL/nullptr to bitfield bitwise operators
    friend checked_bit_field  operator& (const std::nullptr_t, const checked_bit_field&) noexcept { return checked_bit_field(static_cast<word_t>(0)); }
    friend checked_bit_field  operator| (const std::nullptr_t, const checked_bit_field& rhs) noexcept { return checked_bit_field(rhs.word); }
    friend checked_bit_field  operator^ (const std::nullptr_t, const checked_bit_field& rhs) noexcept { return checked_bit_field(rhs.word); }
    friend checked_bit_field  operator& (const std::nullptr_t, const volatile checked_bit_field&) noexcept { return checked_bit_field(static_cast<word_t>(0)); }
    friend checked_bit_field  operator| (const std::nullptr_t, const volatile checked_bit_field& rhs) noexcept { return checked_bit_field(rhs.word); }
    friend checked_bit_field  operator^ (const std::nullptr_t, const volatile checked_bit_field& rhs) noexcept { return checked_bit_field(rhs.word); }
    //bitfield to 0/NULL/nullptr comparison operators
    bool operator==(const std::nullptr_t) const noexcept { return word == 0; }
    bool operator!=(const std::nullptr_t) const noexcept { return word != 0; }
    bool operator<=(const std::nullptr_t) const noexcept { return word <= 0; }
    bool operator>=(const std::nullptr_t) const noexcept { return word >= 0; }
    bool operator< (const std::nullptr_t) const noexcept { return word < 0; }
    bool operator> (const std::nullptr_t) const noexcept { return word > 0; }
    bool operator==(const std::nullptr_t) const volatile noexcept { return word == 0; }
    bool operator!=(const std::nullptr_t) const volatile noexcept { return word != 0; }
    bool operator<=(const std::nullptr_t) const volatile noexcept { return word <= 0; }
    bool operator>=(const std::nullptr_t) const volatile noexcept { return word >= 0; }
    bool operator< (const std::nullptr_t) const volatile noexcept { return word < 0; }
    bool operator> (const std::nullptr_t) const volatile noexcept { return word > 0; }
    //bitfield to 0/NULL/nullptr bitwise operators
    checked_bit_field  operator& (const std::nullptr_t) const noexcept { return checked_bit_field(static_cast<word_t>(0)); }
    checked_bit_field  operator| (const std::nullptr_t) const noexcept { return checked_bit_field(word); }
    checked_bit_field  operator^ (const std::nullptr_t) const noexcept { return checked_bit_field(word); }
    checked_bit_field  operator& (const std::nullptr_t) const volatile noexcept { return checked_bit_field(static_cast<word_t>(0)); }
    checked_bit_field  operator| (const std::nullptr_t) const volatile noexcept { return checked_bit_field(word); }
    checked_bit_field  operator^ (const std::nullptr_t) const volatile noexcept { return checked_bit_field(word); }
    checked_bit_field& operator&=(const std::nullptr_t) noexcept { word &= 0; return *this; }
    checked_bit_field& operator|=(const std::nullptr_t) noexcept { word |= 0; return *this; }
    checked_bit_field& operator^=(const std::nullptr_t) noexcept { word ^= 0; return *this; }
    volatile checked_bit_field& operator&=(const std::nullptr_t) volatile noexcept { word &= 0; return *this; }
    volatile checked_bit_field& operator|=(const std::nullptr_t) volatile noexcept { word |= 0; return *this; }
    volatile checked_bit_field& operator^=(const std::nullptr_t) volatile noexcept { word ^= 0; return *this; }
    //bitfield to bitfield comparison operators
    bool operator==(const checked_bit_field& rhs) const noexcept { return word == rhs.word; }
    bool operator!=(const checked_bit_field& rhs) const noexcept { return word != rhs.word; }
    bool operator<=(const checked_bit_field& rhs) const noexcept { return word <= rhs.word; }
    bool operator>=(const checked_bit_field& rhs) const noexcept { return word >= rhs.word; }
    bool operator< (const checked_bit_field& rhs) const noexcept { return word <  rhs.word; }
    bool operator> (const checked_bit_field& rhs) const noexcept { return word >  rhs.word; }
    bool operator==(const volatile checked_bit_field& rhs) const noexcept { return word == rhs.word; }
    bool operator!=(const volatile checked_bit_field& rhs) const noexcept { return word != rhs.word; }
    bool operator<=(const volatile checked_bit_field& rhs) const noexcept { return word <= rhs.word; }
    bool operator>=(const volatile checked_bit_field& rhs) const noexcept { return word >= rhs.word; }
    bool operator< (const volatile checked_bit_field& rhs) const noexcept { return word <  rhs.word; }
    bool operator> (const volatile checked_bit_field& rhs) const noexcept { return word >  rhs.word; }
    bool operator==(const volatile checked_bit_field& rhs) const volatile noexcept { return word == rhs.word; }
    bool operator!=(const volatile checked_bit_field& rhs) const volatile noexcept { return word != rhs.word; }
    bool operator<=(const volatile checked_bit_field& rhs) const volatile noexcept { return word <= rhs.word; }
    bool operator>=(const volatile checked_bit_field& rhs) const volatile noexcept { return word >= rhs.word; }
    bool operator< (const volatile checked_bit_field& rhs) const volatile noexcept { return word <  rhs.word; }
    bool operator> (const volatile checked_bit_field& rhs) const volatile noexcept { return word >  rhs.word; }
    //bitfield to bitfield bitwise operators
    checked_bit_field  operator~ () const noexcept { return checked_bit_field(~word); }
    checked_bit_field  operator~ () const volatile noexcept { return checked_bit_field(~word); }
    checked_bit_field  operator& (const checked_bit_field& rhs) const noexcept { return checked_bit_field(word & rhs.word); }
    checked_bit_field  operator& (const checked_bit_field& rhs) const volatile noexcept { return checked_bit_field(word & rhs.word); }
    checked_bit_field  operator& (const volatile checked_bit_field& rhs) const noexcept { return checked_bit_field(word & rhs.word); }
    checked_bit_field  operator& (const volatile checked_bit_field& rhs) const volatile noexcept { return checked_bit_field(word & rhs.word); }

    checked_bit_field  operator| (const checked_bit_field& rhs) const noexcept { return checked_bit_field(word | rhs.word); }
    checked_bit_field  operator| (const checked_bit_field& rhs) const volatile noexcept { return checked_bit_field(word | rhs.word); }
    checked_bit_field  operator| (const volatile checked_bit_field& rhs) const noexcept { return checked_bit_field(word | rhs.word); }
    checked_bit_field  operator| (const volatile checked_bit_field& rhs) const volatile noexcept { return checked_bit_field(word | rhs.word); }
    checked_bit_field  operator^ (const checked_bit_field& rhs) const noexcept { return checked_bit_field(word ^ rhs.word); }
    checked_bit_field  operator^ (const checked_bit_field& rhs) const volatile noexcept { return checked_bit_field(word ^ rhs.word); }
    checked_bit_field  operator^ (const volatile checked_bit_field& rhs) const noexcept { return checked_bit_field(word ^ rhs.word); }
    checked_bit_field  operator^ (const volatile checked_bit_field& rhs) const volatile noexcept { return checked_bit_field(word ^ rhs.word); }
    checked_bit_field& operator&=(const checked_bit_field& rhs) noexcept { word &= rhs.word; return *this; }
    checked_bit_field& operator|=(const checked_bit_field& rhs) noexcept { word |= rhs.word; return *this; }
    checked_bit_field& operator^=(const checked_bit_field& rhs) noexcept { word ^= rhs.word; return *this; }
    volatile checked_bit_field& operator&=(const checked_bit_field& rhs) volatile noexcept { word &= rhs.word; return *this; }
    volatile checked_bit_field& operator|=(const checked_bit_field& rhs) volatile noexcept { word |= rhs.word; return *this; }
    volatile checked_bit_field& operator^=(const checked_bit_field& rhs) volatile noexcept { word ^= rhs.word; return *this; }
    volatile checked_bit_field& operator&=(const volatile checked_bit_field& rhs) volatile noexcept { word &= rhs.word; return *this; }
    volatile checked_bit_field& operator|=(const volatile checked_bit_field& rhs) volatile noexcept { word |= rhs.word; return *this; }
    volatile checked_bit_field& operator^=(const volatile checked_bit_field& rhs) volatile noexcept { word ^= rhs.word; return *this; }
    //bitfield to bitmask comparison operators
    bool operator==(const fieldbit_t& rhs) const noexcept { return word == rhs.word; }
    bool operator==(const fieldbit_t& rhs) const volatile noexcept { return word == rhs.word; }
    bool operator!=(const fieldbit_t& rhs) const noexcept { return word != rhs.word; }
    bool operator!=(const fieldbit_t& rhs) const volatile noexcept { return word != rhs.word; }
    bool operator<=(const fieldbit_t& rhs) const noexcept { return word <= rhs.word; }
    bool operator<=(const fieldbit_t& rhs) const volatile noexcept { return word <= rhs.word; }
    bool operator>=(const fieldbit_t& rhs) const noexcept { return word >= rhs.word; }
    bool operator>=(const fieldbit_t& rhs) const volatile noexcept { return word >= rhs.word; }
    bool operator< (const fieldbit_t& rhs) const noexcept { return word < rhs.word; }
    bool operator< (const fieldbit_t& rhs) const volatile noexcept { return word < rhs.word; }
    bool operator> (const fieldbit_t& rhs) const noexcept { return word > rhs.word; }
    bool operator> (const fieldbit_t& rhs) const volatile noexcept { return word > rhs.word; }
    //bitfield to bitmask bitwise operators
    checked_bit_field& operator&=(const fieldbit_t& rhs) noexcept { word &= rhs.word; return *this; }
    checked_bit_field& operator|=(const fieldbit_t& rhs) noexcept { word |= rhs.word; return *this; }
    checked_bit_field& operator^=(const fieldbit_t& rhs) noexcept { word ^= rhs.word; return *this; }
    checked_bit_field  operator& (const fieldbit_t& rhs) const noexcept { return checked_bit_field(word & rhs.word); }
    checked_bit_field  operator& (const fieldbit_t& rhs) const volatile noexcept { return checked_bit_field(word & rhs.word); }
    checked_bit_field  operator| (const fieldbit_t& rhs) const noexcept { return checked_bit_field(word | rhs.word); }
    checked_bit_field  operator| (const fieldbit_t& rhs) const volatile noexcept { return checked_bit_field(word | rhs.word); }
    checked_bit_field  operator^ (const fieldbit_t& rhs) const noexcept { return checked_bit_field(word ^ rhs.word); }
    checked_bit_field  operator^ (const fieldbit_t& rhs) const volatile noexcept { return checked_bit_field(word ^ rhs.word); }

    //shift operators for all integer types
    checked_bit_field  operator<< (const unsigned int s) const noexcept { assert(s <= 8 * sizeof(word_t)); return checked_bit_field(word << s); }
    checked_bit_field  operator<< (const unsigned int s) const volatile noexcept { assert(s <= 8 * sizeof(word_t)); return checked_bit_field(word << s); }
    checked_bit_field  operator>> (const unsigned int s) const noexcept { assert(s <= 8 * sizeof(word_t)); return checked_bit_field(word >> s); }
    checked_bit_field  operator>> (const unsigned int s) const volatile noexcept { assert(s <= 8 * sizeof(word_t)); return checked_bit_field(word >> s); }
    checked_bit_field& operator<<=(const unsigned int s) noexcept { assert(s <= 8 * sizeof(word_t)); word <<= s; return *this; }
    checked_bit_field& operator>>=(const unsigned int s) noexcept { assert(s <= 8 * sizeof(word_t)); word >>= s; return *this; }
    volatile checked_bit_field& operator<<=(const unsigned int s) volatile noexcept { assert(s <= 8 * sizeof(word_t)); word <<= s; return *this; }
    volatile checked_bit_field& operator>>=(const unsigned int s) volatile noexcept { assert(s <= 8 * sizeof(word_t)); word >>= s; return *this; }

    //logical operators
    const bool operator!() const noexcept { return (word == 0 ? true : false); }
    const bool operator!() const volatile noexcept { return (word == 0 ? true : false); }

    //conversion to bool
    /*need explicit as bool can convert to integer (i.e. 0 or 1), that means we can do
    something like checked_bit_field + int unless explicit is declared. Note, explicit means we
    can't do bool truefalse = checked_bit_field. That requires we drop explicit. To get around
    you must do bool truefalse = (bool)checked_bit_field.*/
    explicit operator const bool() const noexcept { return (word != 0 ? true : false); }
    explicit operator const bool() const volatile noexcept { return (word != 0 ? true : false); }

    //deleted operators
    checked_bit_field operator-() = delete;
    checked_bit_field operator++() = delete;
    checked_bit_field operator--() = delete;
    template<class T> checked_bit_field operator+(T)  const = delete;
    template<class T> checked_bit_field operator*(T)  const = delete;
    template<class T> checked_bit_field operator/(T)  const = delete;
    template<class T> checked_bit_field operator%(T)  const = delete;
    template<class T> checked_bit_field operator+=(T) const = delete;
    template<class T> checked_bit_field operator-=(T) const = delete;
    template<class T> checked_bit_field operator*=(T) const = delete;
    template<class T> checked_bit_field operator/=(T) const = delete;
    template<class T> checked_bit_field operator%=(T) const = delete;
    template<class T> checked_bit_field operator+(T)  const volatile = delete;
    template<class T> checked_bit_field operator*(T)  const volatile = delete;
    template<class T> checked_bit_field operator/(T)  const volatile = delete;
    template<class T> checked_bit_field operator%(T)  const volatile = delete;
    template<class T> checked_bit_field operator+=(T) const volatile = delete;
    template<class T> checked_bit_field operator-=(T) const volatile = delete;
    template<class T> checked_bit_field operator*=(T) const volatile = delete;
    template<class T> checked_bit_field operator/=(T) const volatile = delete;
    template<class T> checked_bit_field operator%=(T) const volatile = delete;

#ifdef _MSC_VER
//multiple copy constructors specified
//multiple assignment operators specified
#pragma warning(suppress : 4521 4522)
};
#elif
};
#endif

template <bitfield_unique_id* unique_id, typename word_t>
class checked_bit_mask {
private:
    //the bit mask
    word_t word;

    // private constructor from an integer type.
    explicit checked_bit_mask(word_t init) noexcept : word(init) {}

public:
    //static factory constructor
    //we use a template so we can validate on compile that the bit being set isn't greater than the size of the word
    template <unsigned int i> static constexpr checked_bit_mask set_bit() noexcept
    {
        static_assert(i <= 8 * sizeof(word_t), "bit to set must be within bounds of field");
        /*You may notice that there is a redundant test for inside the shift operator, even though
        the shift can never be evaluated for i == 0. It is not required, but some compilers see
        shift by i-1 and complain that for i == 0 the number is invalid, without checking whether
        or not the shift is actually evaluated.*/
        return checked_bit_mask((i > 0) ? (word_t(1) << ((i > 0) ? (i - 1) : 0)) : 0);
    }
    template <unsigned int i> static constexpr checked_bit_mask set_bits() noexcept
    {
        //2 raised to the power of number of bits - 1, gives the unsigned integer value
        static_assert(i <= (static_cast<word_t>(1) << ((8 * sizeof(word_t)) - 1)), "value is greater than number of bit combinations");
        return checked_bit_mask(i);
    }

    // Corresponding bit field type.
    friend class checked_bit_field<unique_id, word_t>;
    typedef checked_bit_field<unique_id, word_t> field_t;

    //--Constructors--
    //default constructor - our "word" is zeroed
    explicit checked_bit_mask() noexcept : word(0) {}
    //copy constructor
    checked_bit_mask(const checked_bit_mask& rhs) noexcept : word(rhs.word) {}
    checked_bit_mask(const std::nullptr_t) noexcept : word(static_cast<word_t>(0)) {}

    //--Operations--

    //bitmask to bitfield comparison operators
    bool operator==(const field_t& rhs) const noexcept { return word == rhs.word; }
    bool operator==(const volatile field_t& rhs) const noexcept { return word == rhs.word; }
    bool operator!=(const field_t& rhs) const noexcept { return word != rhs.word; }
    bool operator!=(const volatile field_t& rhs) const noexcept { return word != rhs.word; }
    bool operator<=(const field_t& rhs) const noexcept { return word <= rhs.word; }
    bool operator<=(const volatile field_t& rhs) const noexcept { return word <= rhs.word; }
    bool operator>=(const field_t& rhs) const { return word >= rhs.word; }
    bool operator>=(const volatile field_t& rhs) const noexcept { return word >= rhs.word; }
    bool operator< (const field_t& rhs) const { return word < rhs.word; }
    bool operator< (const volatile field_t& rhs) const noexcept { return word < rhs.word; }
    bool operator> (const field_t& rhs) const { return word > rhs.word; }
    bool operator> (const volatile field_t& rhs) const noexcept { return word > rhs.word; }

    //bitmask to bitmask bitwise operators
    constexpr checked_bit_mask operator~() const noexcept { return checked_bit_mask(~word); }
    constexpr checked_bit_mask operator|(const checked_bit_mask& rhs) const noexcept { return checked_bit_mask(word | rhs.word); }
    constexpr checked_bit_mask operator&(const checked_bit_mask& rhs) const noexcept { return checked_bit_mask(word & rhs.word); }
    constexpr checked_bit_mask operator^(const checked_bit_mask& rhs) const noexcept { return checked_bit_mask(word ^ rhs.word); }

    //bitmask to bitfield bitwise operators
    field_t operator|(const field_t& rhs) const noexcept { return field_t(word | rhs.word); }
    field_t operator|(const volatile field_t& rhs) const noexcept { return field_t(word | rhs.word); }
    field_t operator&(const field_t& rhs) const noexcept { return field_t(word & rhs.word); }
    field_t operator&(const volatile field_t& rhs) const noexcept { return field_t(word & rhs.word); }
    field_t operator^(const field_t& rhs) const noexcept { return field_t(word ^ rhs.word); }
    field_t operator^(const volatile field_t& rhs) const noexcept { return field_t(word ^ rhs.word); }
};

// All macros are conditionally defined to use the checked_bit_field classes if _BITFIELD is defined.
//bit field type declaration - BIT_FIELD(long, mybitfield);
#define BIT_FIELD( word_t,  bitfield_t )  extern bitfield_unique_id ui_##bitfield_t; typedef checked_bit_field<&ui_##bitfield_t, word_t> bitfield_t
//bit mask declaration - BIT_MASK(mybitfield, mask1, 0); BIT_MASK(mybitfield, mask2, 1);
#define BIT_MASK( bitfield_t, label, bit_pos ) const bitfield_t::fieldbit_t label = bitfield_t::fieldbit_t::set_bit<bit_pos>()
//bit mask declaration with integer - INT_BIT_MASK(mybitfield, mask1and2, 3)
#define INT_BIT_MASK( bitfield_t, label, int_mask) const bitfield_t::fieldbit_t label = bitfield_t::fieldbit_t::set_bits<int_mask>()
//complex bit constant declaration - BIT_MASKS(mybitfield, complexbitmask) = mask1 | mask2;
#define BIT_MASKS( bitfield_t, label ) const bitfield_t::fieldbit_t label
#else
//bit field type declaration - BIT_FIELD(long, mybitfield);
#define BIT_FIELD(word_t, bitfield_t ) typedef word_t bitfield_t
//bit mask declaration - BIT_MASK(mybitfield, mask1, 0); BIT_MASK(mybitfield, mask2, 1);
inline constexpr unsigned int returnbit(unsigned int i) { return (i > 0) ? (1u << (i - 1)) : 0; };
#define BIT_MASK( bitfield_t, label, bit_pos ) static constexpr bitfield_t label = returnbit(bit_pos)
//bit mask declaration with integer - INT_BIT_MASK(mybitfield, mask1and2, 3)
#define INT_BIT_MASK( bitfield_t, label, int_mask) static constexpr bitfield_t label = int_mask
//complex bit constant declaration - BIT_MASKS(mybitfield, complexbitmask) = mask1 | mask2;
#define BIT_MASKS( field_t, label ) static const field_t label
#endif // SAFE_BIT_FIELD