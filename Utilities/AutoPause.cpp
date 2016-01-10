#include "stdafx.h"
#include "Registry.h"
#include "Emu/System.h"
#include "AutoPause.h"

const extern cfg::bool_entry g_cfg_debug_autopause_syscall("misc/debug/Auto Pause at System Call");
const extern cfg::bool_entry g_cfg_debug_autopause_func_call("misc/debug/Auto Pause at Function Call");

debug::autopause& debug::autopause::get_instance()
{
	// Use magic static
	static autopause instance;
	return instance;
}

// Load Auto Pause Configuration from file "pause.bin"
void debug::autopause::reload(void)
{
	auto& instance = get_instance();

	instance.m_pause_function.clear();
	instance.m_pause_syscall.clear();

	// TODO: better format, possibly a config entry
	if (fs::file list{ fs::get_config_dir() + "pause.bin" })
	{
		u32 num;
		size_t fmax = list.size();
		size_t fcur = 0;
		list.seek(0);
		while (fcur <= fmax - sizeof(u32))
		{
			list.read(&num, sizeof(u32));
			fcur += sizeof(u32);
			if (num == 0xFFFFFFFF) break;
			
			if (num < 1024)
			{
				instance.m_pause_syscall.emplace(num);
				LOG_WARNING(HLE, "Set autopause at syscall %lld", num);
			}
			else
			{
				instance.m_pause_function.emplace(num);
				LOG_WARNING(HLE, "Set autopause at function 0x%08x", num);
			}
		}
	}
}

void debug::autopause::pause_syscall(u64 code)
{
	if (g_cfg_debug_autopause_syscall && get_instance().m_pause_syscall.count(code) != 0)
	{
		Emu.Pause();
		LOG_SUCCESS(HLE, "Autopause triggered at syscall %lld", code);
	}
}

void debug::autopause::pause_function(u32 code)
{
	if (g_cfg_debug_autopause_func_call && get_instance().m_pause_function.count(code) != 0)
	{
		Emu.Pause();
		LOG_SUCCESS(HLE, "Autopause triggered at function 0x%08x", code);
	}
}
