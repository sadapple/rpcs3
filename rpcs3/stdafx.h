#pragma once

#ifdef __INTELLISENSE__
// Silence incorrect intellisense behaviour for std::is_pod<>
#define __is_pod __is_standard_layout
#endif

#ifdef MSVC_CRT_MEMLEAK_DETECTION
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#define NOMINMAX

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#if defined(MSVC_CRT_MEMLEAK_DETECTION) && defined(_DEBUG) && !defined(DBG_NEW)
	#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
	#include "define_new_memleakdetect.h"
#endif

#pragma warning( disable : 4351 )

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <climits>
#include <cmath>
#include <cerrno>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <vector>
#include <queue>
#include <stack>
#include <set>
#include <array>
#include <string>
#include <functional>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <list>
#include <forward_list>
#include <typeindex>
#include <future>
#include <chrono>

#include <restore_new.h>
#define GSL_THROW_ON_CONTRACT_VIOLATION
#include <gsl.h>
#include <define_new_memleakdetect.h>

// Expects() and Ensures() are intended to check function arguments and results.
// Expressions are not guaranteed to evaluate. Redefinition with ASSERT macro for better unification.
#ifdef Expects
#undef Expects
#define Expects ASSERT
#endif

#ifdef Ensures
#undef Ensures
#define Ensures ASSERT
#endif

using namespace std::string_literals;
using namespace std::chrono_literals;

#define IS_LE_MACHINE

#include "Utilities/GNU.h"

#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, "Invalid " #type " type size")
#define CHECK_ALIGN(type, align) static_assert(alignof(type) == align, "Invalid " #type " type alignment")
#define CHECK_MAX_SIZE(type, size) static_assert(sizeof(type) <= size, #type " type size is too big")
#define CHECK_SIZE_ALIGN(type, size, align) CHECK_SIZE(type, size); CHECK_ALIGN(type, align)
#define CHECK_ASCENDING(constexpr_array) static_assert(::is_ascending(constexpr_array), #constexpr_array " is not sorted in ascending order")
#define IS_INTEGRAL(t) (std::is_integral<t>::value || std::is_same<std::decay_t<t>, u128>::value)
#define IS_INTEGER(t) (std::is_integral<t>::value || std::is_enum<t>::value || std::is_same<std::decay_t<t>, u128>::value)
#define IS_BINARY_COMPARABLE(t1, t2) (IS_INTEGER(t1) && IS_INTEGER(t2) && sizeof(t1) == sizeof(t2))

#ifndef _MSC_VER
using u128 = __uint128_t;
#endif

CHECK_SIZE_ALIGN(u128, 16, 16);

#include "Utilities/types.h"

// bool type replacement for PS3/PSV
class b8
{
	std::uint8_t m_value;

public:
	b8() = default;

	constexpr b8(bool value)
		: m_value(value)
	{
	}

	constexpr operator bool() const
	{
		return m_value != 0;
	}
};

CHECK_SIZE_ALIGN(b8, 1, 1);

template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
inline T align(const T& value, u64 align)
{
	return static_cast<T>((value + (align - 1)) & ~(align - 1));
}

// Copy null-terminated string from std::string to char array with truncation
template<std::size_t N>
inline void strcpy_trunc(char(&dst)[N], const std::string& src)
{
	const std::size_t count = src.size() >= N ? N - 1 : src.size();
	std::memcpy(dst, src.c_str(), count);
	dst[count] = '\0';
}

// Copy null-terminated string from char array to another char array with truncation
template<std::size_t N, std::size_t N2>
inline void strcpy_trunc(char(&dst)[N], const char(&src)[N2])
{
	const std::size_t count = N2 >= N ? N - 1 : N2;
	std::memcpy(dst, src, count);
	dst[count] = '\0';
}

// Returns true if all array elements are unique and sorted in ascending order
template<typename T, std::size_t N>
constexpr bool is_ascending(const T(&array)[N], std::size_t from = 0)
{
	return from >= N - 1 ? true : array[from] < array[from + 1] ? is_ascending(array, from + 1) : false;
}

// Get (first) array element equal to `value` or nullptr if not found
template<typename T, std::size_t N, typename T2>
constexpr const T* static_search(const T(&array)[N], const T2& value, std::size_t from = 0)
{
	return from >= N ? nullptr : array[from] == value ? array + from : static_search(array, value, from + 1);
}

// bool wrapper for restricting bool result conversions
struct explicit_bool_t
{
	const bool value;

	constexpr explicit_bool_t(bool value)
		: value(value)
	{
	}

	explicit constexpr operator bool() const
	{
		return value;
	}
};

template<typename T1, typename T2, typename T3 = const char*>
struct triplet_t
{
	T1 first;
	T2 second;
	T3 third;

	constexpr bool operator ==(const T1& right) const
	{
		return first == right;
	}
};

// return 32 bit sizeof() to avoid widening/narrowing conversions with size_t
#define SIZE_32(type) static_cast<u32>(sizeof(type))

// return 32 bit alignof() to avoid widening/narrowing conversions with size_t
#define ALIGN_32(type) static_cast<u32>(alignof(type))

// return 32 bit offsetof()
#define OFFSET_32(type, x) static_cast<u32>(reinterpret_cast<uintptr_t>(&reinterpret_cast<char const volatile&>(reinterpret_cast<type*>(0ull)->x)))

#define CONCATENATE_DETAIL(x, y) x ## y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#define SWITCH(expr) for (const auto& CONCATENATE(_switch_, __LINE__) = (expr);;) { const auto& _switch_ = CONCATENATE(_switch_, __LINE__);
#define CCASE(value) } constexpr std::remove_reference_t<decltype(_switch_)> CONCATENATE(_value_, __LINE__) = value; if (_switch_ == CONCATENATE(_value_, __LINE__)) {
#define VCASE(value) } if (_switch_ == (value)) {
#define DEFAULT      } /* must be the last one */

#define WRAP_EXPR(expr) [&]{ return expr; }
#define COPY_EXPR(expr) [=]{ return expr; }
#define PURE_EXPR(expr) [] { return expr; }

#define HERE "\n(in file " __FILE__ ":" STRINGIZE(__LINE__) ")"

// Obsolete, throw fmt::exception directly. Use HERE macro, if necessary.
#define EXCEPTION(format_str, ...) fmt::exception(format_str HERE, ##__VA_ARGS__)

// Ensure that the expression is evaluated to true. Always evaluated and allowed to have side effects (unlike assert() macro).
#define ASSERT(expr) if (!(expr)) throw std::runtime_error("Assertion failed: " #expr HERE)

#define _PRGNAME_ "RPCS3"
#define _PRGVER_ "0.0.0.6"

#include "Utilities/BEType.h"
#include "Utilities/Atomic.h"
#include "Utilities/StrFmt.h"
#include "Utilities/BitField.h"
#include "Utilities/File.h"
#include "Utilities/Log.h"

// Narrow cast (similar to gsl::narrow) with exception message formatting
template<typename To, typename... Args, typename From>
inline auto narrow(const From& value, const char* format_str/* = "narrow() failed"*/, const Args&... args) -> decltype(static_cast<To>(static_cast<From>(std::declval<To>())))
{
	const auto result = static_cast<To>(value);
	if (static_cast<From>(result) != value) throw fmt::exception(format_str, fmt::do_unveil(args)...);
	return result;
}

// Narrow cast (similar to gsl::narrow) with fixed message (workaround, because default argument fails to compile)
template<typename To, typename From>
inline auto narrow(const From& value) -> decltype(static_cast<To>(static_cast<From>(std::declval<To>())))
{
	const auto result = static_cast<To>(value);
	if (static_cast<From>(result) != value) throw std::runtime_error("narrow() failed");
	return result;
}

// return 32 bit .size() for container
template<typename CT>
inline auto size32(const CT& container) -> decltype(static_cast<u32>(container.size()))
{
	return ::narrow<u32>(container.size(), "size32() failed");
}

// return 32 bit size for an array
template<typename T, std::size_t Size>
constexpr u32 size32(const T(&)[Size])
{
	static_assert(Size <= UINT32_MAX, "size32() error: too big");
	return static_cast<u32>(Size);
}

// Convert 1-byte string to u8 value
constexpr inline u8 operator""_u8(const char* str, std::size_t length)
{
	return length != 1 ? throw std::length_error(str) : static_cast<u8>(str[0]);
}

// Convert 2-byte string to u16 value in native endianness
constexpr inline nse_t<u16> operator""_u16(const char* str, std::size_t length)
{
	return length != 2 ? throw std::length_error(str) :
#ifdef IS_LE_MACHINE
		(static_cast<u8>(str[1]) << 8) | static_cast<u8>(str[0]);
#else
		(static_cast<u8>(str[0]) << 8) | static_cast<u8>(str[1]);
#endif
}

// Convert 4-byte string to u32 value in native endianness
constexpr inline nse_t<u32> operator""_u32(const char* str, std::size_t length)
{
	return length != 4 ? throw std::length_error(str) :
#ifdef IS_LE_MACHINE
		(static_cast<u8>(str[3]) << 24) | (static_cast<u8>(str[2]) << 16) | (static_cast<u8>(str[1]) << 8) | static_cast<u8>(str[0]);
#else
		(static_cast<u8>(str[0]) << 24) | (static_cast<u8>(str[1]) << 16) | (static_cast<u8>(str[2]) << 8) | static_cast<u8>(str[3]);
#endif
}

// Convert 8-byte string to u64 value in native endianness (constexpr fails on gcc)
/*constexpr*/ inline nse_t<u64> operator""_u64(const char* str, std::size_t length)
{
	return length != 8 ? throw std::length_error(str) :
#ifdef IS_LE_MACHINE
		::operator""_u32(str, 4) | (static_cast<u64>(::operator""_u32(str + 4, 4)) << 32);
#else
		::operator""_u32(str + 4, 4) | (static_cast<u64>(::operator""_u32(str, 4)) << 32);
#endif
}

// Some forward declarations for the ID manager
template<typename T>
struct id_traits;

// Bijective mapping between values of enum type T and arbitrary mappable type VT (std::string by default)
template<typename T, typename VT = std::string>
struct enum_map
{
	static_assert(std::is_enum<T>::value, "enum_map<> error: invalid type T (must be enum)");

	// Custom hasher provided for two reasons:
	// 1) To simplify lookup algorithm for MSVC, make it similar everywhere.
	// 2) Workaround for libstdc++ which fails on enum class types.
	struct hash
	{
		std::size_t operator()(T value) const
		{
			return static_cast<std::size_t>(value);
		}
	};

	using init_type = std::initializer_list<std::pair<const T, VT>>;
	using map_type = std::unordered_map<T, VT, hash>;
	using rmap_type = std::unordered_map<VT, T>;
	using list_type = std::vector<VT>;

	static map_type make_map(init_type init)
	{
		map_type map(init.size()); map.reserve(init.size());

		for (const auto& v : init)
		{
			// Ensure elements are unique
			ASSERT(map.emplace(v.first, v.second).second);
		}

		return map;
	}

	static rmap_type make_rmap(init_type init)
	{
		rmap_type map(init.size()); map.reserve(init.size());

		for (const auto& v : init)
		{
			// Ensure elements are unique
			ASSERT(map.emplace(v.second, v.first).second);
		}

		return map;
	}

	static list_type make_list(init_type init)
	{
		list_type list; list.reserve(init.size());

		for (const auto& v : init)
		{
			list.emplace_back(v.second);
		}

		return list;
	}

	const map_type map; // Direct mapping (enum -> type VT)
	const rmap_type rmap; // Reverse mapping (type VT -> enum)
	const list_type list; // List of entries (type VT values) sorted in original order

	enum_map(init_type list)
		: map(make_map(list))
		, rmap(make_rmap(list))
		, list(make_list(list))
	{
		Expects(list.size() != 0);
	}

	// Simple lookup by enum value provided (not expected to fail)
	VT operator[](T value) const
	{
		const auto found = map.find(value);
		Ensures(found != map.end());
		return found->second;
	}
};

// Function template forward declaration which returns enum_map<>.
// Specialize it for specific enum type in appropriate cpp file.
// But don't put it in header, don't use inline or static as well.
// Always search in files for working examples.
template<typename T, typename VT = std::string>
enum_map<T, VT> make_enum_map();

// Retrieve global enum_map<> instance for specified types
template<typename T, typename VT = std::string>
const enum_map<T, VT>& get_enum_map()
{
	// Use magic static
	static const enum_map<T, VT> map = make_enum_map<T, VT>();
	return map;
}
