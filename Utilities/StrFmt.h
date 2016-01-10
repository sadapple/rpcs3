#pragma once

#if defined(_MSC_VER) && _MSC_VER <= 1800
#define snprintf _snprintf
#endif

namespace fmt
{
	std::string replace_first(const std::string& src, const std::string& from, const std::string& to);
	std::string replace_all(const std::string &src, const std::string& from, const std::string& to);

	template<size_t list_size>
	std::string replace_all(std::string src, const std::pair<std::string, std::string>(&list)[list_size])
	{
		for (size_t pos = 0; pos < src.length(); ++pos)
		{
			for (size_t i = 0; i < list_size; ++i)
			{
				const size_t comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
					continue;

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src = (pos ? src.substr(0, pos) + list[i].second : list[i].second) + src.substr(pos + comp_length);
					pos += list[i].second.length() - 1;
					break;
				}
			}
		}

		return src;
	}

	template<size_t list_size>
	std::string replace_all(std::string src, const std::pair<std::string, std::function<std::string()>>(&list)[list_size])
	{
		for (size_t pos = 0; pos < src.length(); ++pos)
		{
			for (size_t i = 0; i < list_size; ++i)
			{
				const size_t comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
					continue;

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src = (pos ? src.substr(0, pos) + list[i].second() : list[i].second()) + src.substr(pos + comp_length);
					pos += list[i].second().length() - 1;
					break;
				}
			}
		}

		return src;
	}

	std::string to_hex(u64 value, u64 count = 1);
	std::string to_udec(u64 value);
	std::string to_sdec(s64 value);

	template<typename T, typename = void>
	struct unveil
	{
		using result_type = T;

		force_inline static result_type get_value(const T& arg)
		{
			return arg;
		}
	};

	template<>
	struct unveil<const char*>
	{
		using result_type = const char* const;

		force_inline static result_type get_value(const char* const& arg)
		{
			return arg;
		}
	};

	template<std::size_t N>
	struct unveil<char[N]>
	{
		using result_type = const char* const;

		force_inline static result_type get_value(const char(&arg)[N])
		{
			return arg;
		}
	};

	template<>
	struct unveil<std::string>
	{
		using result_type = const char*;

		force_inline static result_type get_value(const std::string& arg)
		{
			return arg.c_str();
		}
	};

	template<typename T>
	struct unveil<T, std::enable_if_t<std::is_enum<T>::value>>
	{
		using result_type = std::underlying_type_t<T>;

		force_inline static result_type get_value(const T& arg)
		{
			return static_cast<result_type>(arg);
		}
	};

	template<typename T, bool Se>
	struct unveil<se_t<T, Se>>
	{
		using result_type = typename unveil<T>::result_type;

		force_inline static result_type get_value(const se_t<T, Se>& arg)
		{
			return unveil<T>::get_value(arg);
		}
	};

	template<typename T>
	force_inline typename unveil<T>::result_type do_unveil(const T& arg)
	{
		return unveil<T>::get_value(arg);
	}

	// Formatting function with special functionality:
	//
	// std::string is forced to .c_str()
	// be_t<> is forced to .value() (fmt::do_unveil reverts byte order automatically)
	//
	// External specializations for fmt::do_unveil (can be found in another headers):
	// vm::ptr, vm::bptr, ... (fmt::do_unveil) (vm_ptr.h) (with appropriate address type, using .addr() can be avoided)
	// vm::ref, vm::bref, ... (fmt::do_unveil) (vm_ref.h)
	//
	template<typename... Args>
	safe_buffers std::string format(const char* fmt, const Args&... args)
	{
		// fixed stack buffer for the first attempt
		std::array<char, 4096> fixed_buf;

		// possibly dynamically allocated buffer for the second attempt
		std::unique_ptr<char[]> buf;

		// pointer to the current buffer
		char* buf_addr = fixed_buf.data();

		for (std::size_t buf_size = fixed_buf.size();;)
		{
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
			const std::size_t len = std::snprintf(buf_addr, buf_size, fmt, do_unveil(args)...);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
			if (len > INT_MAX)
			{
				throw std::runtime_error("std::snprintf() failed");
			}

			if (len < buf_size)
			{
				return{ buf_addr, len };
			}

			buf.reset(buf_addr = new char[buf_size = len + 1]);
		}
	}

	// Create exception of type T (std::runtime_error by default) with formatting
	template<typename T = std::runtime_error, typename... Args>
	never_inline safe_buffers T exception(const char* fmt, const Args&... args) noexcept(noexcept(T{ fmt }))
	{
		return T{ format(fmt, do_unveil(args)...).c_str() };
	}

	// Create exception of type T (std::runtime_error by default) without formatting
	template<typename T = std::runtime_error>
	safe_buffers T exception(const char* msg) noexcept(noexcept(T{ msg }))
	{
		return T{ msg };
	}

	std::vector<std::string> split(const std::string& source, std::initializer_list<std::string> separators, bool is_skip_empty = true);
	std::string trim(const std::string& source, const std::string& values = " \t");

	template<typename T>
	std::string merge(const T& source, const std::string& separator)
	{
		if (!source.size())
		{
			return{};
		}

		std::string result;

		auto it = source.begin();
		auto end = source.end();
		for (--end; it != end; ++it)
		{
			result += *it + separator;
		}

		return result + source.back();
	}

	template<typename T>
	std::string merge(std::initializer_list<T> sources, const std::string& separator)
	{
		if (!sources.size())
		{
			return{};
		}

		std::string result;
		bool first = true;

		for (auto &v : sources)
		{
			if (first)
			{
				result = fmt::merge(v, separator);
				first = false;
			}
			else
			{
				result += separator + fmt::merge(v, separator);
			}
		}

		return result;
	}

	std::string tolower(std::string source);
	std::string toupper(std::string source);
	std::string escape(const std::string& source, std::initializer_list<char> more = {});
	std::string unescape(const std::string& source);
	bool match(const std::string &source, const std::string &mask);
}
