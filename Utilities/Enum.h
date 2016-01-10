#pragma once

// Aux: compare two strings
constexpr bool _enum_str_equal(const char* left, const char* right)
{
	return *left != *right ? false : *left == '\0' ? true : _enum_str_equal(left + 1, right + 1);
}

struct enum_literal
{
	const char* str;

	constexpr enum_literal(const char* str)
		: str(str)
	{
	}
};

// Aux: search in string array
template<std::size_t M>
constexpr std::size_t _enum_search(const enum_literal(&table)[M], const char* value, std::size_t from = 0)
{
	return from >= M ? throw EXCEPTION("Invalid value: %s", value) : _enum_str_equal(table[from].str, value) ? from : _enum_search(table, value, from + 1);
}

// "String" enum type base:
// VT: enum value provider
template<typename VT>
class string_enum
{
	std::size_t m_value;

public:
	using vtype = VT;

	static constexpr std::size_t max = sizeof(VT::values) / sizeof(enum_literal);

	static constexpr std::size_t to_value(const char* str)
	{
		return _enum_search(VT::values, str);
	}

	string_enum() = default;

	// Initialization, example: constexpr text_enum x = "42";
	constexpr string_enum(const char* str)
		: m_value(to_value(str))
	{
	}

	string_enum& operator =(const string_enum&) = default;

	// Assignment, example: test_enum x; x = "42";
	string_enum& operator =(const char* str)
	{
		m_value = to_value(str);
		return *this;
	}

	// Get numeric value
	constexpr std::size_t index() const
	{
		return m_value < max ? m_value : throw EXCEPTION("Invalid data: [%p] 0x%llx", this, m_value);
	}

	// Get value string
	constexpr const char* c_str() const
	{
		return VT::values[index()].str;
	}

	std::string str() const
	{
		return c_str();
	}

	friend constexpr bool operator ==(const string_enum& left, const char* str)
	{
		return to_value(str) == left.m_value;
	}

	friend constexpr bool operator ==(const char* str, const string_enum& right)
	{
		return to_value(str) == right.m_value;
	}

	constexpr bool operator ==(const string_enum& right) const
	{
		return m_value == right.m_value;
	}

	friend constexpr bool operator !=(const string_enum& left, const char* str)
	{
		return to_value(str) != left.m_value;
	}

	friend constexpr bool operator !=(const char* str, const string_enum& right)
	{
		return to_value(str) != right.m_value;
	}

	constexpr bool operator !=(const string_enum& right) const
	{
		return m_value != right.m_value;
	}
};

template<typename VT>
constexpr std::size_t string_enum<VT>::max;
