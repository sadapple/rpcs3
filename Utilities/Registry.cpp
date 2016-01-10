#include "stdafx.h"
#include "Thread.h"
#include "Registry.h"

namespace cfg
{
	const extern _log::channel log("CFG");

	static registry& get_registry()
	{
		try
		{
			// Use magic static
			static registry config(fs::get_config_dir() + "config.ini");
			return config;
		}
		catch (...)
		{
			catch_all_exceptions();
		}
	}

	// Property placeholder, contains string value
	struct raw_property final : property_base
	{
		std::string value;

		raw_property(const std::string& value)
			: value(value)
		{
		}

		type get_type() const override
		{
			return type::unknown;
		}

		std::string to_string() const override
		{
			return value;
		}

		bool validate(const std::string& value) const override
		{
			return true;
		}

		bool from_string(const std::string& value) override
		{
			this->value = value;
			return true;
		}

		void from_default() override
		{
			// Do nothing (default value is unknown)
		}

		bool is_default() const override
		{
			return false; // TODO: is it correct?
		}
	};
}

cfg::registry::registry(const std::string& path)
	: m_main_file(path, fom::read | fom::write | fom::create)
{
	if (m_main_file)
	{
		// Load config
		for (auto&& v : deserialize(m_main_file.to_string()))
		{
			// Initialize raw property handlers
			m_map.emplace(v.first, std::make_unique<raw_property>(std::move(v.second)));
		}
	}
}

void cfg::registry::save()
{
	std::lock_guard<shared_mutex> lock(m_mutex);

	if (m_main_file && m_layers.empty())
	{
		map_type props;

		// Store config
		for (const auto& v : m_map)
		{
			// Store only default values (test)
			if (!v.second->is_default())
				props.emplace(v.first, v.second->to_string());
		}

		m_main_file.seek(0);
		m_main_file.trunc(0);
		m_main_file.write(serialize(props));
	}
}

cfg::registry::map_type cfg::registry::deserialize(const std::string& text)
{
	cfg::registry::map_type result;

	// Current group name
	std::string group;

	for (auto it = text.begin(), end = text.end(); it != end;)
	{
		// Characters 0..31 are considered delimiters
		if ((*it & ~31) == 0)
		{
			it++;
			continue;
		}
		
		// Get string range
		auto str_start = it;		
		while (it != end && *it & ~31) it++;
		auto str_end = it;

		// Analyse
		if (*str_start == '[' && *(str_end - 1) == ']')
		{
			// Set new group name
			group = fmt::unescape({ str_start + 1, str_end - 1 });
		}
		else
		{
			// Find delimiter
			auto delim = str_start;
			while (delim != str_end && *delim != '=') delim++;

			if (delim != str_end)
			{
				// Get property name, value
				std::string&& name = fmt::unescape({ str_start, delim });

				// Add property
				result.emplace(group.empty() ? name : group + '/' + name, fmt::unescape({ delim + 1, str_end }));
			}
		}
	}

	return result;
}

std::string cfg::registry::serialize(const cfg::registry::map_type& map)
{
	std::map<std::string, std::map<std::string, std::string>> groups;

	// Sort properties in groups
	for (const auto& p : map)
	{
		const auto& p_name = p.first.substr(p.first.find_last_of('/') + 1);
		const auto& g_name = p.first.substr(0, p_name.size() == p.first.size() ? 0 : p.first.size() - p_name.size() - 1);

		groups[g_name][p_name] = p.second;
	}

	std::string result;

	// Write groups
	for (const auto& g : groups)
	{
		// Write escaped group name
		if (g.first.size())
		{
			result += '[';
			result += fmt::escape(g.first);
			result += "]\n";
		}

		// Write escaped properties
		for (const auto& p : g.second)
		{
			// Escape also '=' and '[' characters in property name
			result += fmt::escape(p.first, { '=', '[' });
			result += '=';
			result += fmt::escape(p.second);
			result += '\n';
		}

		result += '\n';
	}

	return result;
}

void cfg::registry::set_property_handler(const std::string& name, std::unique_ptr<property_base> data)
{
	std::lock_guard<shared_mutex> lock(m_mutex);

	const auto found = m_map.find(name);

	if (found != m_map.end())
	{
		// Preserve previous value
		const std::string& value = found->second->to_string();
		const type old_type = found->second->get_type();

		if (old_type != type::unknown)
		{
			log.error("add_property('%s'): already registered with type %d", name, old_type);
		}

		log.trace("add_property('%s'): old value %s", name, value);
		found->second.reset();

		// Try to restore value
		if (!data->from_string(value))
		{
			log.warning("add_property('%s'): failed to restore value %s", name, value);
		}

		// Install new property handler
		log.trace("add_property('%s'): property replaced with value %s", name, data->to_string());
		found->second = std::move(data);
	}
	else
	{
		log.trace("add_property('%s'): new property added with value %s", name, data->to_string());
		m_map.emplace(name, std::move(data));
	}
}

void cfg::registry::remove_property_handler(const std::string& name)
{
	std::lock_guard<shared_mutex> lock(m_mutex);

	const auto found = m_map.find(name);

	// Compare registered property with a pointer provided (if not null)
	if (found != m_map.end())
	{
		// Preserve previous value
		const std::string& value = found->second->to_string();

		if (found->second->get_type() == type::unknown)
		{
			log.error("remove_property('%s'): not registered", name);
		}

		// Install default property handler with original value
		found->second = std::make_unique<raw_property>(value);
	}
	else
	{
		log.error("remove_property('%s'): not found", name);
	}
}

std::string cfg::registry::to_string(const std::string& name) const
{
	reader_lock lock(m_mutex);

	const auto found = m_map.find(name);

	if (found != m_map.end())
	{
		auto&& value = found->second->to_string();
		log.trace("to_string('%s'): read value %s", name, value);
		return value;
	}

	log.warning("to_string('%s'): not found", name);
	return{};
}

cfg::type cfg::registry::get_type(const std::string& name) const
{
	reader_lock lock(m_mutex);

	const auto found = m_map.find(name);

	if (found != m_map.end())
	{
		const type type = found->second->get_type();
		log.trace("get_type('%s'): obtained type %d", name, type);
		return type;
	}

	log.warning("get_type('%s'): not found", name);
	return type::unknown;
}

std::vector<std::string> cfg::registry::get_values(const std::string& name) const
{
	reader_lock lock(m_mutex);

	const auto found = m_map.find(name);

	if (found != m_map.end())
	{
		auto&& values = found->second->get_values();
		log.trace("get_values('%s'): obtained values (size=%zu)", name, values.size());
		return values;
	}

	log.warning("get_values('%s'): not found", name);
	return{};
}

bool cfg::registry::from_string(const std::string& name, const std::string& value)
{
	std::lock_guard<shared_mutex> lock(m_mutex);

	if (auto& prop = m_map[name])
	{
		const bool result = prop->from_string(value);
		log.trace("from_string('%s'): %s with value %s", name, result ? "succeeded" : "failed", value);
		return result;
	}
	else
	{
		// Create raw property if not registered
		log.notice("from_string('%s'): raw property created with value %s", name, value);
		prop = std::make_unique<raw_property>(value);
	}
	
	return true;
}

void cfg::save()
{
	get_registry().save();
}

void cfg::register_property(const std::string& name, std::unique_ptr<property_base> data)
{
	get_registry().set_property_handler(name, std::move(data));
}

void cfg::unregister_property(const std::string& name)
{
	get_registry().remove_property_handler(name);
}

std::string cfg::to_string(const std::string& name)
{
	return get_registry().to_string(name);
}

bool cfg::to_bool(const std::string& name, bool def)
{
	const std::string& value = get_registry().to_string(name);

	if (value == "false")
		return false;
	if (value == "true")
		return true;

	return def;
}

cfg::type cfg::get_type(const std::string& name)
{
	return get_registry().get_type(name);
}

std::vector<std::string> cfg::get_values(const std::string& name)
{
	return get_registry().get_values(name);
}

std::size_t cfg::to_index(const std::string& name, std::size_t def)
{
	const std::vector<std::string>& list = get_registry().get_values(name);
	const std::string& value = get_registry().to_string(name);

	for (std::size_t i = 0; i < list.size(); i++)
	{
		if (list[i] == value)
		{
			return i;
		}
	}

	return def;
}

bool cfg::from_string(const std::string& name, const std::string& value)
{
	return get_registry().from_string(name, value);
}

bool cfg::from_bool(const std::string& name, bool value)
{
	return get_registry().from_string(name, value ? "true" : "false");
}

bool cfg::from_index(const std::string& name, std::size_t index)
{
	return get_registry().from_string(name, get_registry().get_values(name).at(index));
}

bool cfg::try_to_int64(s64* out, const std::string& value, s64 min, s64 max)
{
	// TODO: could be rewritten without exceptions (but it shall be as safe as possible and provide logs)
	s64 result;
	std::size_t pos;

	try
	{
		result = std::stoll(value, &pos, 0 /* Auto-detect numeric base */);
	}
	catch (const std::exception& e)
	{
		if (out) log.error("cfg::try_to_int('%s'): exception: %s", value, e.what());
		return false;
	}

	if (pos != value.size())
	{
		if (out) log.error("cfg::try_to_int('%s'): unexpected characters (pos=%zu)", value, pos);
		return false;
	}

	if (result < min || result > max)
	{
		if (out) log.error("cfg::try_to_int('%s'): out of bounds (%lld..%lld)", value, min, max);
		return false;
	}

	if (out) *out = result;
	return true;
}
