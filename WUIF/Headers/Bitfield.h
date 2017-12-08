/*Copyright 2017 Jonathan Campbell

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
#ifdef _DEBUG
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
    explicit checked_bit_field(word_t init) : word(init) {}

public:
    //For convenience with macros, we declare checked_bit_field::fieldbit_t
    typedef checked_bit_mask<unique_id, word_t> fieldbit_t;
    friend class checked_bit_mask<unique_id, word_t>;

    //--Constructors--
    //default constructor - our "word" is zeroed
    explicit checked_bit_field() : word(0) {}
    //copy constructor
    checked_bit_field(const checked_bit_field& rhs) : word(rhs.word) {}
    //copy constructor from bit mask
    checked_bit_field(const fieldbit_t& rhs) : word(rhs.word) {}
    //copy constructor from 0
    checked_bit_field(const std::nullptr_t& rhs) : word(0) { UNREFERENCED_PARAMETER(rhs); }

    //--Copy Assignments--
    //copy assignment operator
    checked_bit_field& operator=(const checked_bit_field& rhs) { if (this != &rhs) word = rhs.word; return *this; }
    //copy assignment operator from bit mask
    checked_bit_field& operator=(const fieldbit_t& rhs) { word = rhs.word; return *this; }
    //assignment operator to allow for assigning 0 (clearing bits)
    checked_bit_field& operator=(const std::nullptr_t& rhs) { word = 0; return *this; }


    //--Operations--

    //0/NULL/nullptr to bitfield comparison operators
    friend bool operator==(std::nullptr_t, checked_bit_field) { return word == 0; }
    friend bool operator!=(std::nullptr_t, checked_bit_field) { return word != 0; }
    friend bool operator<=(std::nullptr_t, checked_bit_field) { return word <= 0; }
    friend bool operator>=(std::nullptr_t, checked_bit_field) { return word >= 0; }
    friend bool operator< (std::nullptr_t, checked_bit_field) { return word <  0; }
    friend bool operator> (std::nullptr_t, checked_bit_field) { return word >  0; }
    //bitfield to 0/NULL/nullptr comparison operators
    bool operator==(std::nullptr_t) const { return word == 0; }
    bool operator!=(std::nullptr_t) const { return word != 0; }
    bool operator<=(std::nullptr_t) const { return word <= 0; }
    bool operator>=(std::nullptr_t) const { return word >= 0; }
    bool operator< (std::nullptr_t) const { return word <  0; }
    bool operator> (std::nullptr_t) const { return word >  0; }
    //bitfield to bitfield comparison operators
    bool operator==(const checked_bit_field& rhs) const { return word == rhs.word; }
    bool operator!=(const checked_bit_field& rhs) const { return word != rhs.word; }
    bool operator<=(const checked_bit_field& rhs) const { return word <= rhs.word; }
    bool operator>=(const checked_bit_field& rhs) const { return word >= rhs.word; }
    bool operator< (const checked_bit_field& rhs) const { return word <  rhs.word; }
    bool operator> (const checked_bit_field& rhs) const { return word >  rhs.word; }
    //bitfield to bitmask comparison operators
    bool operator==(const fieldbit_t& rhs) const { return word == rhs.word; }
    bool operator!=(const fieldbit_t& rhs) const { return word != rhs.word; }
    bool operator<=(const fieldbit_t& rhs) const { return word <= rhs.word; }
    bool operator>=(const fieldbit_t& rhs) const { return word >= rhs.word; }
    bool operator< (const fieldbit_t& rhs) const { return word <  rhs.word; }
    bool operator> (const fieldbit_t& rhs) const { return word >  rhs.word; }
    //bitfield to bitfield bitwise operators
    checked_bit_field  operator~ () const { return checked_bit_field(~word); }
    checked_bit_field& operator&=(const checked_bit_field& rhs) { word &= rhs.word; return *this; }
    checked_bit_field& operator|=(const checked_bit_field& rhs) { word |= rhs.word; return *this; }
    checked_bit_field& operator^=(const checked_bit_field& rhs) { word ^= rhs.word; return *this; }
    checked_bit_field  operator& (const checked_bit_field& rhs) const { return checked_bit_field(word & rhs.word); }
    checked_bit_field  operator| (const checked_bit_field& rhs) const { return checked_bit_field(word | rhs.word); }
    checked_bit_field  operator^ (const checked_bit_field& rhs) const { return checked_bit_field(word ^ rhs.word); }

    //bitfield to bitmask bitwise operators
    checked_bit_field& operator&=(const fieldbit_t& rhs) { word &= rhs.word; return *this; }
    checked_bit_field& operator|=(const fieldbit_t& rhs) { word |= rhs.word; return *this; }
    checked_bit_field& operator^=(const fieldbit_t& rhs) { word ^= rhs.word; return *this; }
    checked_bit_field  operator& (const fieldbit_t& rhs) const { return checked_bit_field(word & rhs.word); }
    checked_bit_field  operator| (const fieldbit_t& rhs) const { return checked_bit_field(word | rhs.word); }
    checked_bit_field  operator^ (const fieldbit_t& rhs) const { return checked_bit_field(word ^ rhs.word); }

    //shift operators for all integer types
    checked_bit_field& operator<<=(unsigned int s) { static_assert(s <= 8 * sizeof(word_t), "shift larger than word"); word <<= s; return *this; }
    checked_bit_field& operator>>=(unsigned int s) { static_assert(s <= 8 * sizeof(word_t), "shift larger than word"); word >>= s; return *this; }
    checked_bit_field  operator<< (unsigned int s) const { static_assert(s <= 8 * sizeof(word_t), "shift larger than word"); return checked_bit_field(word << s); }
    checked_bit_field  operator>> (unsigned int s) const { static_assert(s <= 8 * sizeof(word_t), "shift larger than word"); return checked_bit_field(word >> s); }

    bool operator!() { if (word == 0) return true; else return false; }
    //conversion to bool - need explicit as bool can convert to integer (i.e. 0 or 1)
    explicit operator bool() const { if (word == 0) return false; else return true; }
};

template <bitfield_unique_id* unique_id, typename word_t>
class checked_bit_mask {
private:
    //the bit mask
    word_t word;

    // private constructor from an integer type.
    explicit checked_bit_mask(word_t init) : word(init) {}

public:
    //static factory constructor
    //we use a template so we can validate on compile that the bit being set isn't greater than the size of the word
    template <unsigned int i> static checked_bit_mask set_bit()
    {
        static_assert(i <= 8 * sizeof(word_t), "bit to set must be within bounds of field");
        /*You may notice that there is a redundant test for inside the shift operator, even though
        the shift can never be evaluated for i == 0. It is not required, but some compilers see
        shift by i-1 and complain that for i == 0 the number is invalid, without checking whether
        or not the shift is actually evaluated.*/
        return checked_bit_mask((i > 0) ? (word_t(1) << ((i > 0) ? (i - 1) : 0)) : 0);
    }
    template <unsigned int i> static checked_bit_mask set_bits()
    {
        static_assert(i <= ((2 << (8 * sizeof(word_t))) - 1), "value is greater than number of bit combinations"); //2 raised to the power of number of bits - 1, gives the unsigned interger value
        return checked_bit_mask(i);
    }

    // Corresponding bit field type.
    typedef checked_bit_field<unique_id, word_t> field_t;
    friend class checked_bit_field<unique_id, word_t>;

    //--Constructors--
    //default constructor - our "word" is zeroed
    explicit checked_bit_mask() : word(0) {}
    //copy constructor
    checked_bit_mask(const checked_bit_mask& rhs) : word(rhs.word) {}


    //--Operations--

    //bitmask to bitfield comparison operators
    bool operator==(const field_t& rhs) const { return word == rhs.word; }
    bool operator!=(const field_t& rhs) const { return word != rhs.word; }
    bool operator<=(const field_t& rhs) const { return word <= rhs.word; }
    bool operator>=(const field_t& rhs) const { return word >= rhs.word; }
    bool operator< (const field_t& rhs) const { return word <  rhs.word; }
    bool operator> (const field_t& rhs) const { return word >  rhs.word; }

    //bitfield to bitfield bitwise operators
    checked_bit_mask operator~() const { return checked_bit_mask(~word); }
    checked_bit_mask operator|(const checked_bit_mask& rhs) const { return checked_bit_mask(word | rhs.word); }
    checked_bit_mask operator&(const checked_bit_mask& rhs) const { return checked_bit_mask(word & rhs.word); }
    checked_bit_mask operator^(const checked_bit_mask& rhs) const { return checked_bit_mask(word ^ rhs.word); }

    //bitmask to bitfield bitwise operators
    field_t operator|(const field_t& rhs) const { return field_t(word | rhs.word); }
    field_t operator&(const field_t& rhs) const { return field_t(word & rhs.word); }
    field_t operator^(const field_t& rhs) const { return field_t(word ^ rhs.word); }
};


// All macros are conditionally defined to use the checked_bit_field classes if _DEBUG is defined.
//bit field type declaration - BIT_FIELD(long, mybitfield);
#define BIT_FIELD( word_t,  bitfield_t ) extern bitfield_unique_id ui_##bitfield_t; typedef checked_bit_field<&ui_##bitfield_t, word_t> bitfield_t
//bit mask declaration - BIT_MASK(mybitfield, mask1, 0); BIT_MASK(mybitfield, mask2, 1);
#define BIT_MASK( bitfield_t, label, bit_pos ) const bitfield_t::fieldbit_t label = bitfield_t::fieldbit_t::set_bit<bit_pos>()
//bit mask declaration with integer - INT_BIT_MASK(mybitfield, mask1and2, 3)
#define INT_BIT_MASK( bitfield_t, label, int_mask) const bitfield_t::fieldbit_t label = bitfield_t::fieldbit_t::set_bits<int_mask>()
//complex bit constant declaration - BIT_MASKS(mybitfield, complexbitmask) = mask1 | mask2;
#define BIT_MASKS( bitfield_t, label ) const field_t::fieldbit_t label
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