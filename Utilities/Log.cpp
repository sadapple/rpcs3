#include "stdafx.h"
#include "Registry.h"
#include "Thread.h"
#include "File.h"
#include "Log.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace _log
{
	static logger& get_logger()
	{
		// Use magic static
		static logger instance;
		return instance;
	}

	file_listener g_log_file(_PRGNAME_ ".log");

	file_writer g_tty_file("TTY.log");

	channel GENERAL("", level::notice);
	channel LOADER("LDR", level::notice);
	channel MEMORY("MEM", level::notice);
	channel RSX("RSX", level::notice);
	channel HLE("HLE", level::notice);
	channel PPU("PPU", level::notice);
	channel SPU("SPU", level::notice);
	channel ARMv7("ARMv7");
}

template<>
enum_map<_log::level> make_enum_map()
{
	return
	{
		{ _log::level::always, "Nothing" },
		{ _log::level::fatal, "Fatal" },
		{ _log::level::error, "Error" },
		{ _log::level::todo, "TODO" },
		{ _log::level::success, "Success" },
		{ _log::level::warning, "Warning" },
		{ _log::level::notice, "Notice" },
		{ _log::level::trace, "Trace" },
	};
}

_log::listener::listener()
{
	// Register self
	get_logger().add_listener(this);
}

_log::listener::~listener()
{
	// Unregister self
	get_logger().remove_listener(this);
}

_log::channel::channel(const std::string& name, _log::level init_level)
	: name{ name }
	, enabled{ init_level }
{
	struct property : cfg::property_base
	{
		std::atomic<level>& value;
		const level def;
		const enum_map<level>& emap = get_enum_map<level>();

		property(std::atomic<level>& value, level def)
			: value(value)
			, def(def)
		{
		}

		cfg::type get_type() const override
		{
			return cfg::type::enumeration;
		}

		std::vector<std::string> get_values() const override
		{
			return emap.list;
		}

		std::string to_string() const override
		{
			return emap[value];
		}

		bool validate(const std::string& name) const override
		{
			return emap.rmap.count(name) != 0;
		}

		bool from_string(const std::string& name) override
		{
			const auto found = emap.rmap.find(name);

			if (found != emap.rmap.end())
			{
				value = found->second;
				return true;
			}

			return false;
		}

		void from_default() override
		{
			value = def;
		}

		bool is_default() const override
		{
			return value == def;
		}
	};

	cfg::register_property("log/" + name, std::make_unique<property>(enabled, init_level));
}

_log::channel::~channel()
{
	cfg::unregister_property("log/" + name);
}

void _log::logger::add_listener(_log::listener* listener)
{
	std::lock_guard<shared_mutex> lock(m_mutex);

	m_listeners.emplace(listener);
}

void _log::logger::remove_listener(_log::listener* listener)
{
	std::lock_guard<shared_mutex> lock(m_mutex);

	m_listeners.erase(listener);
}

void _log::logger::broadcast(const _log::channel& ch, _log::level sev, const std::string& text) const
{
	reader_lock lock(m_mutex);

	for (auto listener : m_listeners)
	{
		listener->log(ch, sev, text);
	}
}

void _log::broadcast(const _log::channel& ch, _log::level sev, const std::string& text)
{
	get_logger().broadcast(ch, sev, text);
}

_log::file_writer::file_writer(const std::string& name)
{
	try
	{
		if (!m_file.open(fs::get_config_dir() + name, fom::rewrite | fom::append))
		{
			throw EXCEPTION("Can't create log file %s (error %d)", name, errno);
		}
	}
	catch (const std::exception& e)
	{
#ifdef _WIN32
		MessageBoxA(0, e.what(), "_log::file_writer() failed", MB_ICONERROR);
#else
		std::printf("_log::file_writer() failed: %s\n", e.what());
#endif
	}
}

void _log::file_writer::log(const std::string& text)
{
	m_file.write(text);
}

std::size_t _log::file_writer::size() const
{
	return m_file.pos();
}

void _log::file_listener::log(const _log::channel& ch, _log::level sev, const std::string& text)
{
	std::string msg; msg.reserve(text.size() + 200);

	// Used character: U+00B7 (Middle Dot)
	switch (sev)
	{
	case level::always:  msg = u8"·A "; break;
	case level::fatal:   msg = u8"·F "; break;
	case level::error:   msg = u8"·E "; break;
	case level::todo:    msg = u8"·U "; break;
	case level::success: msg = u8"·S "; break;
	case level::warning: msg = u8"·W "; break;
	case level::notice:  msg = u8"·! "; break;
	case level::trace:   msg = u8"·T "; break;
	}

	// TODO: print time?

	if (auto t = thread_ctrl::get_current())
	{
		msg += '{';
		msg += t->get_name();
		msg += "} ";
	}

	if (ch.name.size())
	{
		msg += ch.name;
		msg += sev == level::todo ? " TODO: " : ": ";
	}
	else if (sev == level::todo)
	{
		msg += "TODO: ";
	}
	
	msg += text;
	msg += '\n';

	file_writer::log(msg);
}
