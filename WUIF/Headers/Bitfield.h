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
#pragma once
//see http://www.artima.com/cppsource/safelabels.html for the details on this solution

template <unsigned int unique_index, typename word_t>
class safe_bit_const;

template <unsigned int unique_index, typename word_t>
class safe_bit_field
{

public:

	// Corresponding constant bit field type.
	typedef safe_bit_const<unique_index, word_t> const_t;
	friend class safe_bit_const<unique_index, word_t>;

	// Default constructor, allows to construct bit fields on the stack.
	safe_bit_field() : word(0) {}

	// Copy constructor and assignment operators.
	safe_bit_field(const safe_bit_field& rhs) : word(rhs.word) {}
	safe_bit_field operator=(safe_bit_field rhs) { word = rhs.word; return *this; }

	// Copy constructor and assignment operators from constant bit fields.
	safe_bit_field(const const_t& rhs) : word(rhs.word) {}
	safe_bit_field operator=(const_t rhs) { word = rhs.word; return *this; }

	// Comparison operators.
	bool operator==(safe_bit_field rhs) const { return word == rhs.word; }
	bool operator!=(safe_bit_field rhs) const { return word != rhs.word; }
	bool operator< (safe_bit_field rhs) const { return word < rhs.word; }
	bool operator> (safe_bit_field rhs) const { return word > rhs.word; }
	bool operator<=(safe_bit_field rhs) const { return word <= rhs.word; }
	bool operator>=(safe_bit_field rhs) const { return word >= rhs.word; }
	//
	bool operator==(const_t rhs) const { return word == rhs.word; }
	bool operator!=(const_t rhs) const { return word != rhs.word; }
	bool operator< (const_t rhs) const { return word < rhs.word; }
	bool operator> (const_t rhs) const { return word > rhs.word; }
	bool operator<=(const_t rhs) const { return word <= rhs.word; }
	bool operator>=(const_t rhs) const { return word >= rhs.word; }

	// Bitwise operations.
	safe_bit_field operator|(safe_bit_field rhs) const { return safe_bit_field(word | rhs.word); }
	safe_bit_field operator&(safe_bit_field rhs) const { return safe_bit_field(word & rhs.word); }
	safe_bit_field operator^(safe_bit_field rhs) const { return safe_bit_field(word ^ rhs.word); }
	safe_bit_field operator~() const { return safe_bit_field(~word); }
	safe_bit_field operator|=(safe_bit_field rhs) { word |= rhs.word; return safe_bit_field(*this); }
	safe_bit_field operator&=(safe_bit_field rhs) { word &= rhs.word; return safe_bit_field(*this); }
	safe_bit_field operator^=(safe_bit_field rhs) { word ^= rhs.word; return safe_bit_field(*this); }
	//
	safe_bit_field operator|(const_t rhs) const { return safe_bit_field(word | rhs.word); }
	safe_bit_field operator&(const_t rhs) const { return safe_bit_field(word & rhs.word); }
	safe_bit_field operator^(const_t rhs) const { return safe_bit_field(word ^ rhs.word); }
	safe_bit_field operator|=(const_t rhs) { word |= rhs.word; return safe_bit_field(*this); }
	safe_bit_field operator&=(const_t rhs) { word &= rhs.word; return safe_bit_field(*this); }
	safe_bit_field operator^=(const_t rhs) { word ^= rhs.word; return safe_bit_field(*this); }

	// Conversion to bool.
	operator const bool() const { return word; }

	// Shift operators shift bits inside the bit field.
	safe_bit_field operator<< (unsigned int s) { return safe_bit_field(word << s); }
	safe_bit_field operator>> (unsigned int s) { return safe_bit_field(word >> s); }
	safe_bit_field operator<<=(unsigned int s) { word <<= s; return *this; }
	safe_bit_field operator>>=(unsigned int s) { word >>= s; return *this; }

	// Word size is also the maximum number of different bit fields for a given word type.
	static size_t size() { return sizeof(word_t); }

	// Type of the bit field is available if needed.
	typedef word_t bit_word_t;

private:

	// Bits - the real data.
	word_t word;

	// Private constructor from an integer type. Don't put too much stock into
	// explicit declaration, it's better than nothing but
	// does not solve all problems with undesired conversions because of
	// direct initialization.
	explicit safe_bit_field(word_t init) : word(init) {}

	// Here comes the interesting stuff: all the operators designed to trap
	// unintended conversions and make them not compile.
	template <typename T> safe_bit_field operator|(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	template <typename T> safe_bit_field operator&(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	template <typename T> safe_bit_field operator^(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	template <typename T> safe_bit_field operator|=(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	template <typename T> safe_bit_field operator&=(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	template <typename T> safe_bit_field operator^=(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	//

	template <typename T> bool operator==(const T) const {
		Forbidden_conversion<T>
			forbidden_conversion; return true;
	}
	template <typename T> bool operator!=(const T) const {
		Forbidden_conversion<T> forbidden_conversion; return true;
	}
	template <typename T> bool operator<(const T) const {
		Forbidden_conversion<T>
			forbidden_conversion; return true;
	}
	template <typename T> bool operator>(const T) const {
		Forbidden_conversion<T> forbidden_conversion; return true;
	}
	template <typename T> bool operator<=(const T) const {
		Forbidden_conversion<T> forbidden_conversion; return true;
	}
	template <typename T> bool operator>=(const T) const {
		Forbidden_conversion<T> forbidden_conversion; return true;
	}
};

// Non-member template operators to catch errors where safe label is not the first argument.
template <unsigned int unique_index, typename word_t>
inline safe_bit_field<unique_index, word_t> operator&(bool, safe_bit_field<unique_index, word_t> rhs)
{
	Forbidden_conversion<word_t> forbidden_conversion; return rhs;
}
template <unsigned int unique_index, typename word_t>
inline safe_bit_field<unique_index, word_t> operator|(bool, safe_bit_field<unique_index, word_t> rhs)
{
	Forbidden_conversion<word_t> forbidden_conversion; return rhs;
}
template <unsigned int unique_index, typename word_t>
inline safe_bit_field<unique_index, word_t> operator^(bool, safe_bit_field<unique_index, word_t> rhs)
{
	Forbidden_conversion<word_t> forbidden_conversion; return rhs;
}
template <unsigned int unique_index, typename word_t>
inline safe_bit_field<unique_index, word_t> operator==(bool, safe_bit_field<unique_index, word_t> rhs)
{
	Forbidden_conversion<word_t> forbidden_conversion; return rhs;
}
template <unsigned int unique_index, typename word_t>
inline safe_bit_field<unique_index, word_t> operator!=(bool, safe_bit_field<unique_index, word_t> rhs)
{
	Forbidden_conversion<word_t> forbidden_conversion; return rhs;
}


template <unsigned int unique_index, typename word_t = unsigned long>
class safe_bit_const {

public:

	// Static factory constructor.
	template <unsigned int i> static safe_bit_const make_bit_const()
	{

		ASSERT(i <= 8 * sizeof(word_t));
		ASSERT(sizeof(safe_bit_const) == sizeof(word_t));
		/*You may notice that there is a redundant test for inside the shift operator, even though
		the shift can never be evaluated for i == 0. It is not required, but some compilers see
		shift by i-1 and complain that for i == 0 the number is invalid, without checking whether
		or not the shift is actually evaluated.*/
		return safe_bit_const((i > 0) ? (word_t(1) << ((i > 0) ? (i - 1) : 0)) : 0);
	}

	// Corresponding bit field type.
	typedef safe_bit_field<unique_index, word_t> field_t;
	friend class safe_bit_field<unique_index, word_t>;

	// Copy constructor and assignment operators.
	safe_bit_const(const safe_bit_const& rhs) : word(rhs.word) {}
	safe_bit_const operator=(safe_bit_const rhs) { word = rhs.word; return *this; }

	// Comparison operators.
	bool operator==(safe_bit_const rhs) const { return word == rhs.word; }
	bool operator!=(safe_bit_const rhs) const { return word != rhs.word; }
	bool operator< (safe_bit_const rhs) const { return word < rhs.word; }
	bool operator> (safe_bit_const rhs) const { return word > rhs.word; }
	bool operator<=(safe_bit_const rhs) const { return word <= rhs.word; }
	bool operator>=(safe_bit_const rhs) const { return word >= rhs.word; }
	//
	bool operator==(field_t rhs) const { return word == rhs.word; }
	bool operator!=(field_t rhs) const { return word != rhs.word; }
	bool operator< (field_t rhs) const { return word < rhs.word; }
	bool operator> (field_t rhs) const { return word > rhs.word; }
	bool operator<=(field_t rhs) const { return word <= rhs.word; }
	bool operator>=(field_t rhs) const { return word >= rhs.word; }

	// Bitwise operations.
	safe_bit_const operator|(safe_bit_const rhs) const { return safe_bit_const(word | rhs.word); }
	safe_bit_const operator&(safe_bit_const rhs) const { return safe_bit_const(word & rhs.word); }
	safe_bit_const operator^(safe_bit_const rhs) const { return safe_bit_const(word ^ rhs.word); }
	safe_bit_const operator~() const { return safe_bit_const(~word); }
	safe_bit_const operator|=(safe_bit_const rhs) { word |= rhs.word; return safe_bit_const(*this); }
	safe_bit_const operator&=(safe_bit_const rhs) { word &= rhs.word; return safe_bit_const(*this); }
	safe_bit_const operator^=(safe_bit_const rhs) { word ^= rhs.word; return safe_bit_const(*this); }
	//

	field_t operator|(field_t rhs) const { return field_t(word | rhs.word); }
	field_t operator&(field_t rhs) const { return field_t(word & rhs.word); }
	field_t operator^(field_t rhs) const { return field_t(word ^ rhs.word); }

	// Shift operators shift bits inside the bit field. Does not make sense, most of
	// the time, except perhaps to loop over bit fields and increment them.
	safe_bit_const operator<< (unsigned int s) { return safe_bit_const(word << s); }
	safe_bit_const operator>> (unsigned int s) { return safe_bit_const(word >> s); }
	safe_bit_const operator<<=(unsigned int s) { word <<= s; return *this; }
	safe_bit_const operator>>=(unsigned int s) { word >>= s; return *this; }

	// Word size is also the maximum number of different bit fields for a given word type.
	static size_t size() { return sizeof(word_t); }

	// Type of the bit field is available if needed.
	typedef word_t bit_word_t;

	////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////
private:

	// Private constructor from an integer type.
	explicit safe_bit_const(word_t init) : word(init) {}

	// Bits - the real data.
	word_t word;

	// Here comes the interesting stuff: all the operators designed to trap
	// unintended conversions and make them not compile.
	// Operators below handle code like this:
	// safe_bit_const<1> label1;
	// safe_bit_const<2> label2;
	// if ( label1 & label2 ) { ... }
	//
	// These operators are private, and will not instantiate in any event because of
	//the incomplete forbidden_conversion struct.
	template <typename T> safe_bit_const operator|(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	template <typename T> safe_bit_const operator&(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	template <typename T> safe_bit_const operator^(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	template <typename T> safe_bit_const operator|=(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	template <typename T> safe_bit_const operator&=(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}
	template <typename T> safe_bit_const operator^=(T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return *this;
	}

	// And the same thing for comparisons:
	// if ( label1 == label2 ) { ... }
	template <typename T> bool operator==(const T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return true;
	}
	template <typename T> bool operator!=(const T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return true;
	}
	template <typename T> bool operator<(const T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return true;
	}
	template <typename T> bool operator>(const T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return true;
	}
	template <typename T> bool operator<=(const T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return true;
	}
	template <typename T> bool operator>=(const T) const
	{
		Forbidden_conversion<T> forbidden_conversion; return true;
	}
};


// All macros are conditionally defined to use the safe_bit_field classes if
// SAFE_BIT_FIELD is defined.
#ifdef _DEBUG
#define SAFE_BIT_FIELD
#endif

// Field type declaration.
/*You may notice that there is a redundant test for inside the shift operator, even though
the shift can never be evaluated for i == 0. It is not required, but some compilers see
shift by i-1 and complain that for i == 0 the number is invalid, without checking whether
or not the shift is actually evaluated.*/
#ifdef SAFE_BIT_FIELD
#define BIT_FIELD(u_index, word_t ) typedef safe_bit_field<u_index, word_t>
#else
#define BIT_FIELD(u_index, word_t ) typedef word_t
#endif // SAFE_BIT_FIELD

// Bit constant declaration.
#ifdef SAFE_BIT_FIELD
#define BIT_CONST( field_t, label, bit_index ) static const field_t::const_t label = field_t::const_t::make_bit_const<bit_index>()
#else
constexpr size_t make_bit_const(size_t i) { return (i > 0) ? (size_t(1) << ((i > 0) ? (i - 1) : 0)) : 0; }
#define BIT_CONST( field_t, label, bit_index ) static constexpr field_t label =static_cast<field_t>(make_bit_const(bit_index))
#endif // SAFE_BIT_FIELD

// Complex bit constant declaration.
#ifdef SAFE_BIT_FIELD
#define BIT_CONSTS( field_t, label ) static const field_t::const_t label
#else
#define BIT_CONSTS( field_t, label ) static const field_t label
#endif // SAFE_BIT_FIELD

// Maximum number of bits for a given type:
#ifdef SAFE_BIT_FIELD
#define BIT_FIELD_COUNT( field_t ) field_t::size()
#else
#define BIT_FIELD_COUNT( field_t ) (8*sizeof(field_t))
#endif // SAFE_BIT_FIELD