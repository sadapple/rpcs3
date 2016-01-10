#pragma once

#include "SharedMutex.h"

namespace cfg
{
	struct property_base;

	// Property type classifier
	enum class type : uint
	{
		unknown = 0, // type for raw_property only ("unregistered" properties)
		enumeration, // fixed set of valid values (can be obtained by get_values() call)
		boolean, // usually associated with bool type, string values are "false" or "true"
		string, // unrestricted string value (must be used for any unclassified exotic type)
		int64, // signed integer
	};

	class registry
	{
	public:
		// Configuration map type (values only)
		using map_type = std::unordered_map<std::string, std::string>;

		// Configuration layer (TODO)
		struct layer
		{
			map_type prev; // Previous configuration
			fs::file file;
		};

	private:
		mutable shared_mutex m_mutex;

		// Registered property handlers
		std::unordered_map<std::string, std::unique_ptr<property_base>> m_map;

		// Main config file
		fs::file m_main_file;

		// Supplementary configuration layers
		std::stack<layer> m_layers;

	public:
		// Initialization
		registry(const std::string& path);
		void save();

		/* Serialization */

		static map_type deserialize(const std::string& text);
		static std::string serialize(const map_type& map);

		/* Property registration functions */

		void set_property_handler(const std::string& name, std::unique_ptr<property_base>);
		void remove_property_handler(const std::string& name);

		/* Property access functions */

		std::string to_string(const std::string& name) const;
		type get_type(const std::string& name) const;
		std::vector<std::string> get_values(const std::string& name) const;
		bool from_string(const std::string& name, const std::string& value);
	};

	// Register property with full name and property handler
	void register_property(const std::string& name, std::unique_ptr<property_base>);

	// Unregister property
	void unregister_property(const std::string& name);

	// Save configuration
	void save();

	// Get property value (string)
	std::string to_string(const std::string& name);

	// Get property value (bool)
	bool to_bool(const std::string& name, bool def = false);

	// Get property type
	type get_type(const std::string& name);

	// Get list of valid property values
	std::vector<std::string> get_values(const std::string& name);

	// Get property value index
	std::size_t to_index(const std::string& name, std::size_t def = 0);

	// Set string value
	bool from_string(const std::string& name, const std::string& value);

	// Set bool value
	bool from_bool(const std::string& name, bool value);

	// Set value by index
	bool from_index(const std::string& name, std::size_t index);

	// Property handler virtual abstract base class
	struct property_base
	{
		virtual ~property_base() = default;

		// Set value from string (default if failed)
		explicit_bool_t operator =(const std::string& value)
		{
			return from_string(value) || (from_default(), false);
		}

		// Get property type (type::unknown for raw_property, anything else for other derivatives)
		virtual type get_type() const = 0;

		// Get list of valid property values (optional)
		virtual std::vector<std::string> get_values() const
		{
			return{};
		}

		// Convert to string
		virtual std::string to_string() const = 0;

		// Check whether the string is a valid value
		virtual bool validate(const std::string& value) const = 0;

		// Try to set value from string
		virtual bool from_string(const std::string& value) = 0;

		// Set default value
		virtual void from_default() = 0;

		// Check whether the current value is equal to default one
		virtual bool is_default() const = 0;
	};

	// Aux conversion function: try to get integer value
	bool try_to_int64(s64* result, const std::string& value, s64 min = INT64_MIN, s64 max = INT64_MAX);

	// Aux conversion function: try to get two integer values
	template<typename T, char Delimiter = 'x', typename CT = void>
	bool try_to_int_pair(CT* result, const std::string& value, s64 min = std::numeric_limits<T>::min(), s64 max = std::numeric_limits<T>::max())
	{
		const auto dpos = value.find(Delimiter);

		s64 v1, v2;

		if (value.empty() ||
			!try_to_int64(result ? &v1 : nullptr, value.substr(0, dpos), min, max) ||
			!try_to_int64(result ? &v2 : nullptr, value.substr(dpos + 1, value.size() - (dpos + 1)), min, max))
		{
			return false;
		}

		if (result) *result = CT{ static_cast<T>(v1), static_cast<T>(v2) };
		return true;
	}

	template<typename T>
	struct map_entry final
	{
		using init_type = std::initializer_list<std::pair<std::string, T>>;
		using map_type = std::unordered_map<std::string, T>;
		using iterator = typename map_type::const_iterator;
		using list_type = std::vector<std::string>;

		static map_type make_map(init_type init)
		{
			map_type map(init.size());

			for (const auto& v : init)
			{
				// Ensure elements are unique
				ASSERT(map.emplace(v.first, v.second).second);
			}

			return map;
		}

		static list_type make_list(init_type init)
		{
			list_type list; list.reserve(init.size());

			for (const auto& v : init)
			{
				list.emplace_back(v.first);
			}

			return list;
		}

	private:
		iterator m_value;

	public:
		const std::string name;
		const map_type map; // for search
		const list_type list; // elements sorted in original order provided
		const iterator def; // default value

		// Overload with a default value specified as a string
		map_entry(const std::string& name, const std::string& def, init_type init)
			: name(name)
			, map(make_map(init))
			, list(make_list(init))
			, def(map.find(def))
		{
			// Check default value
			if (this->def == map.end()) throw EXCEPTION("cfg::map_entry('%s'): invalid default value '%s'", name, def);

			// Set initial value to default
			m_value = this->def;

			struct property : property_base
			{
				map_entry& entry;

				property(map_entry* ptr)
					: entry(*ptr)
				{
				}

				type get_type() const override
				{
					return type::enumeration;
				}

				std::vector<std::string> get_values() const override
				{
					return entry.list;
				}

				std::string to_string() const override
				{
					return entry.m_value->first;
				}

				bool validate(const std::string& value) const override
				{
					return entry.map.count(value) != 0;
				}

				bool from_string(const std::string& value) override
				{
					const auto found = entry.map.find(value);

					if (found == entry.map.end())
					{
						return false;
					}
					else
					{
						entry.m_value = found;
						return true;
					}
				}

				void from_default() override
				{
					entry.m_value = entry.def;
				}

				bool is_default() const override
				{
					return entry.m_value == entry.def;
				}
			};

			register_property(name, std::make_unique<property>(this));
		}

		// Overload with a default value specified as the index
		map_entry(const std::string& name, std::size_t def, init_type list)
			: map_entry(name, def < list.size() ? (list.begin() + def)->first : throw EXCEPTION("cfg::map_entry('%s'): invalid default value index %zu", name, def), list)
		{
		}

		// Overload with a default value as first value
		map_entry(const std::string& name, init_type list)
			: map_entry(name, 0, list)
		{
		}

		~map_entry()
		{
			unregister_property(name);
		}

		const T& get() const
		{
			return m_value->second;
		}

		std::string to_string() const
		{
			return m_value->first;
		}

		// Value cannot be changed in current implementation
	};

	template<typename T>
	struct enum_entry final
	{
		std::atomic<T> value;
		const T def;
		const std::string name;

		enum_entry(const std::string& name, T def = {})
			: value(def)
			, name(name)
			, def(def)
		{
			struct property : property_base
			{
				enum_entry& entry;
				const enum_map<T>& emap = get_enum_map<T>();

				property(enum_entry* ptr)
					: entry(*ptr)
				{
				}

				type get_type() const override
				{
					return type::enumeration;
				}

				std::vector<std::string> get_values() const override
				{
					return emap.list;
				}

				std::string to_string() const override
				{
					return emap[entry.value];
				}

				bool validate(const std::string& value) const override
				{
					return emap.rmap.count(value) != 0;
				}

				bool from_string(const std::string& value) override
				{
					const auto found = emap.rmap.find(value);

					if (found != emap.rmap.end())
					{
						entry.value = found->second;
						return true;
					}

					return false;
				}

				void from_default() override
				{
					entry.value = entry.def;
				}

				bool is_default() const override
				{
					return entry.value == entry.def;
				}
			};

			register_property(name, std::make_unique<property>(this));
		}

		~enum_entry()
		{
			unregister_property(name);
		}

		operator T() const
		{
			return value.load();
		}

		enum_entry& operator =(T value)
		{
			this->value = value;
			return *this;
		}
	};

	struct bool_entry final
	{
		std::atomic<bool> value;
		const bool def;
		const std::string name;

		bool_entry(const std::string& name, bool def = false)
			: value(def)
			, name(name)
			, def(def)
		{
			struct property : property_base
			{
				bool_entry& entry;

				property(bool_entry* ptr)
					: entry(*ptr)
				{
				}

				type get_type() const override
				{
					return type::boolean;
				}

				std::string to_string() const override
				{
					return entry.value.load() ? "true" : "false";
				}

				bool validate(const std::string& value) const override
				{
					return value == "false" || value == "true";
				}

				bool from_string(const std::string& value) override
				{
					if (value == "false")
					{
						entry.value = false;
						return true;
					}
					if (value == "true")
					{
						entry.value = true;
						return true;
					}
					return false;
				}

				void from_default() override
				{
					entry.value = entry.def;
				}

				bool is_default() const override
				{
					return entry.value == entry.def;
				}
			};

			register_property(name, std::make_unique<property>(this));
		}

		~bool_entry()
		{
			unregister_property(name);
		}

		operator bool() const
		{
			return value.load();
		}

		bool_entry& operator =(bool value)
		{
			this->value = value;
			return *this;
		}
	};

	// Signed 64-bit integer entry (supporting different smaller integer types seems unnecessary)
	template<s64 Min = INT64_MIN, s64 Max = INT64_MAX>
	struct int_entry final
	{
		// Use smaller type if possible
		using int_type = std::conditional_t<Min >= INT32_MIN && Max <= INT32_MAX, s32, s64>;

		static_assert(Min < Max, "Invalid cfg::int_entry range");

		std::atomic<int_type> value;
		const int_type def;
		const std::string name;

		int_entry(const std::string& name, int_type def = std::min<int_type>(Max, std::max<int_type>(Min, 0)))
			: value(def)
			, name(name)
			, def(def)
		{
			struct property : property_base
			{
				int_entry& entry;

				property(int_entry* ptr)
					: entry(*ptr)
				{
				}

				type get_type() const override
				{
					return type::int64;
				}

				std::vector<std::string> get_values() const override
				{
					return{ std::to_string(Min), std::to_string(Max) };
				}

				std::string to_string() const override
				{
					return std::to_string(entry.value.load());
				}

				bool validate(const std::string& value) const override
				{
					return try_to_int64(nullptr, value, Min, Max);
				}

				bool from_string(const std::string& value) override
				{
					s64 result;
					if (try_to_int64(&result, value, Min, Max))
					{
						entry.value = static_cast<int_type>(result);
						return true;
					}

					return false;
				}

				void from_default() override
				{
					entry.value = entry.def;
				}

				bool is_default() const override
				{
					return entry.value == entry.def;
				}
			};

			register_property(name, std::make_unique<property>(this));
		}

		~int_entry()
		{
			unregister_property(name);
		}

		operator int_type() const
		{
			return value.load();
		}

		int_entry& operator =(int_type value)
		{
			// TODO: does it need validation there?
			this->value = value;
			return *this;
		}
	};

	class string_entry final
	{
		mutable std::mutex m_mutex;
		std::string m_value;

	public:
		const std::string def;
		const std::string name;

		string_entry(const std::string& name, const std::string& def = {})
			: m_value(def)
			, def(def)
			, name(name)
		{
			struct property : property_base
			{
				string_entry& entry;

				property(string_entry* ptr)
					: entry(*ptr)
				{
				}

				type get_type() const override
				{
					return type::string;
				}

				std::string to_string() const override
				{
					return entry;
				}

				bool validate(const std::string& value) const override
				{
					return true;
				}

				bool from_string(const std::string& value) override
				{
					entry = value;
					return true;
				}

				void from_default() override
				{
					entry = entry.def;
				}

				bool is_default() const override
				{
					return entry.def.compare(entry) == 0;
				}
			};

			register_property(name, std::make_unique<property>(this));
		}

		~string_entry()
		{
			unregister_property(name);
		}

		operator std::string() const
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_value;
		}

		std::string get() const
		{
			return *this;
		}

		string_entry& operator =(const std::string& value)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_value = value;
			return *this;
		}
	};
}
