#include "stdafx.h"
#include "Utilities/Registry.h"
#include "Utilities/AutoPause.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/events.h"

#include "Emu/GameInfo.h"
#include "Emu/SysCalls/ModuleManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUInstrTable.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/SysCalls/Callback.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/GSRender.h"
#include "Emu/FS/VFS.h"

#include "Loader/PSF.h"
#include "Loader/ELF64.h"
#include "Loader/ELF32.h"

#include "../Crypto/unself.h"

const extern cfg::bool_entry g_cfg_autostart("misc/Always start after boot");
const extern cfg::bool_entry g_cfg_autoexit("misc/Exit RPCS3 when process finishes");

extern cfg::string_entry g_cfg_vfs_dev_hdd0;
extern cfg::string_entry g_cfg_vfs_dev_hdd1;
extern cfg::string_entry g_cfg_vfs_dev_flash;
extern cfg::string_entry g_cfg_vfs_dev_usb000;
extern cfg::string_entry g_cfg_vfs_dev_bdvd;
extern cfg::string_entry g_cfg_vfs_app_home;

using namespace PPU_instr;

static const std::string& BreakPointsDBName = "BreakPoints.dat";
static const u16 bpdb_version = 0x1000;

// Draft (not used)
struct bpdb_header_t
{
	le_t<u32> magic;
	le_t<u32> version;
	le_t<u32> count;
	le_t<u32> marked;

	// POD
	bpdb_header_t() = default;

	bpdb_header_t(u32 count, u32 marked)
		: magic(*reinterpret_cast<const u32*>("BPDB"))
		, version(0x00010000)
		, count(count)
		, marked(marked)
	{
	}
};

extern std::atomic<u32> g_thread_count;

extern u64 get_system_time();
extern void finalize_psv_modules();

Emulator::Emulator()
	: m_status(Stopped)
	, m_mode(DisAsm)
	, m_rsx_callback(0)
	, m_cpu_thr_stop(0)
	, m_thread_manager(new CPUThreadManager())
	, m_callback_manager(new CallbackManager())
	, m_module_manager(new ModuleManager())
{
	m_loader.register_handler(new loader::handlers::elf32);
	m_loader.register_handler(new loader::handlers::elf64);
}

void Emulator::Init()
{
	//rpcs3::config.load();
	rpcs3::oninit();
}

void Emulator::SetPath(const std::string& path, const std::string& elf_path)
{
	m_path = path;
	m_elf_path = elf_path;
}

void Emulator::CreateConfig(const std::string& name)
{
	//const std::string& path = fs::get_config_dir() + "data/" + name;
	//const std::string& ini_file = path + "/settings.ini";

	//if (!fs::is_dir("data"))
	//	fs::create_dir("data");

	//if (!fs::is_dir(path))
	//	fs::create_dir(path);

	//if (!fs::is_file(ini_file))
	//	rpcs3::config_t{ ini_file }.save();
}

bool Emulator::BootGame(const std::string& path, bool direct)
{
	static const char* boot_list[] =
	{
		"/PS3_GAME/USRDIR/BOOT.BIN",
		"/USRDIR/BOOT.BIN",
		"/BOOT.BIN",
		"/PS3_GAME/USRDIR/EBOOT.BIN",
		"/USRDIR/EBOOT.BIN",
		"/EBOOT.BIN",
	};

	if (direct && fs::is_file(path))
	{
		SetPath(path);
		Load();

		return true;
	}

	for (std::string elf : boot_list)
	{
		elf = path + elf;

		if (fs::is_file(elf))
		{
			SetPath(elf);
			Load();

			return true;
		}
	}

	return false;
}

void Emulator::Load()
{
	m_status = Ready;

	if (!fs::is_file(m_path))
	{
		m_status = Stopped;
		return;
	}

	const std::string& elf_dir = fs::get_parent_dir(m_path);

	if (IsSelf(m_path))
	{
		const std::size_t elf_ext_pos = m_path.find_last_of('.');
		const std::string& elf_ext = fmt::toupper(m_path.substr(elf_ext_pos != -1 ? elf_ext_pos : m_path.size()));
		const std::string& elf_name = m_path.substr(elf_dir.size());

		if (elf_name.compare(elf_name.find_last_of("/\\", -1, 2) + 1, 9, "EBOOT.BIN", 9) == 0)
		{
			m_path.erase(m_path.size() - 9, 1); // change EBOOT.BIN to BOOT.BIN
		}
		else if (elf_ext == ".SELF" || elf_ext == ".SPRX")
		{
			m_path.erase(m_path.size() - 4, 1); // change *.self to *.elf, *.sprx to *.prx
		}
		else
		{
			m_path += ".decrypted.elf";
		}

		if (!DecryptSelf(m_path, elf_dir + elf_name))
		{
			m_status = Stopped;
			return;
		}
	}

	ResetInfo();

	LOG_NOTICE(LOADER, "Loading '%s'...", m_path.c_str());

	if (m_elf_path.empty())
	{
		m_elf_path = "/host_root/" + m_path;

		LOG_NOTICE(LOADER, "Elf path: %s", m_elf_path);
		LOG_NOTICE(LOADER, "");
	}

	// "Mount" /dev_bdvd/ (hack)
	if (fs::is_file(elf_dir + "/../../PS3_DISC.SFB"))
	{
		const auto dir_list = fmt::split(elf_dir, { "/", "\\" });

		// Check latest two directories
		if (dir_list.size() >= 2 && dir_list.back() == "USRDIR" && *(dir_list.end() - 2) == "PS3_GAME")
		{
			g_cfg_vfs_dev_bdvd = elf_dir.substr(0, elf_dir.length() - 15);
		}
		else
		{
			g_cfg_vfs_dev_bdvd = elf_dir + "/../../";
		}
	}
	else
	{
		g_cfg_vfs_dev_bdvd = {};
	}

	// "Mount" /app_home/ (hack)
	g_cfg_vfs_app_home = elf_dir + '/';

	// Load PARAM.SFO
	m_psf = psf::load(fs::file(elf_dir + "/../PARAM.SFO"));
	m_title = psf::get_string(m_psf, "TITLE", m_path);
	m_title_id = psf::get_string(m_psf, "TITLE_ID");
	LOG_NOTICE(LOADER, "Title: %s", GetTitle());
	LOG_NOTICE(LOADER, "Serial: %s", GetTitleID());
	LOG_NOTICE(LOADER, "");

	//rpcs3::state.config = rpcs3::config;

	// load custom config
	//if (!rpcs3::config.misc.use_default_ini.value())
	//{
	//	if (title_id.size())
	//	{
	//		title_id = title_id.substr(0, 4) + "-" + title_id.substr(4, 5);
	//		CreateConfig(title_id);
	//		rpcs3::config_t custom_config { fs::get_config_dir() + "data/" + title_id + "/settings.ini" };
	//		custom_config.load();
	//		rpcs3::state.config = custom_config;
	//	}
	//}

	//LOG_NOTICE(LOADER, "Used configuration: '%s'", rpcs3::state.config.path().c_str());
	//LOG_NOTICE(LOADER, "");
	//LOG_NOTICE(LOADER, rpcs3::state.config.to_string().c_str());

	LOG_NOTICE(LOADER, "Mount info:");
	LOG_NOTICE(LOADER, "/dev_hdd0/ -> %s", g_cfg_vfs_dev_hdd0.get());
	LOG_NOTICE(LOADER, "/dev_hdd1/ -> %s", g_cfg_vfs_dev_hdd1.get());
	LOG_NOTICE(LOADER, "/dev_flash/ -> %s", g_cfg_vfs_dev_flash.get());
	LOG_NOTICE(LOADER, "/dev_usb/ -> /dev_usb000/ -> %s", g_cfg_vfs_dev_usb000.get());
	LOG_NOTICE(LOADER, "/dev_bdvd/ -> %s", g_cfg_vfs_dev_bdvd.get());
	LOG_NOTICE(LOADER, "/app_home/ -> %s", g_cfg_vfs_app_home.get());
	LOG_NOTICE(LOADER, "");

	fs::file elf(m_path);
	if (!elf)
	{
		LOG_ERROR(LOADER, "Opening '%s' failed", m_path);
		m_status = Stopped;
		return;
	}

	if (!m_loader.load(elf))
	{
		LOG_ERROR(LOADER, "Loading '%s' failed", m_path);
		m_status = Stopped;
		vm::close();
		return;
	}
	
	LoadPoints(fs::get_config_dir() + BreakPointsDBName);

	fxm::import<GSRender>(PURE_EXPR(Emu.GetCallbacks().get_gs_render())); // Must be created in appropriate sys_rsx syscall
	GetCallbackManager().Init();
	debug::autopause::reload();

	SendDbgCommand(DID_READY_EMU);

	if (g_cfg_autostart)
	{
		Run();
	}
}

void Emulator::Run()
{
	if (!IsReady())
	{
		Load();
		if(!IsReady()) return;
	}

	if (IsRunning()) Stop();

	if (IsPaused())
	{
		Resume();
		return;
	}

	rpcs3::onstart();

	SendDbgCommand(DID_START_EMU);

	m_pause_start_time = 0;
	m_pause_amend_time = 0;
	m_status = Running;

	GetCPU().Exec();
	SendDbgCommand(DID_STARTED_EMU);
}

bool Emulator::Pause()
{
	const u64 start = get_system_time();

	// try to set Paused status
	if (!sync_bool_compare_and_swap(&m_status, Running, Paused))
	{
		return false;
	}

	rpcs3::onpause();

	// update pause start time
	if (m_pause_start_time.exchange(start))
	{
		LOG_ERROR(GENERAL, "Emulator::Pause() error: concurrent access");
	}

	SendDbgCommand(DID_PAUSE_EMU);

	for (auto& t : GetCPU().GetAllThreads())
	{
		t->sleep(); // trigger status check
	}

	SendDbgCommand(DID_PAUSED_EMU);

	return true;
}

void Emulator::Resume()
{
	// get pause start time
	const u64 time = m_pause_start_time.exchange(0);

	// try to increment summary pause time
	if (time)
	{
		m_pause_amend_time += get_system_time() - time;
	}

	// try to resume
	if (!sync_bool_compare_and_swap(&m_status, Paused, Running))
	{
		return;
	}

	if (!time)
	{
		LOG_ERROR(GENERAL, "Emulator::Resume() error: concurrent access");
	}

	SendDbgCommand(DID_RESUME_EMU);

	for (auto& t : GetCPU().GetAllThreads())
	{
		t->awake(); // untrigger status check and signal
	}

	rpcs3::onstart();

	SendDbgCommand(DID_RESUMED_EMU);
}

extern std::map<u32, std::string> g_armv7_dump;

void Emulator::Stop()
{
	LOG_NOTICE(GENERAL, "Stopping emulator...");

	if (sync_lock_test_and_set(&m_status, Stopped) == Stopped)
	{
		return;
	}

	rpcs3::onstop();
	SendDbgCommand(DID_STOP_EMU);

	{
		LV2_LOCK;

		// notify all threads
		for (auto& t : GetCPU().GetAllThreads())
		{
			std::lock_guard<std::mutex> lock(t->mutex);

			t->sleep(); // trigger status check

			t->cv.notify_one(); // signal
		}
	}

	LOG_NOTICE(GENERAL, "All threads signaled...");

	while (g_thread_count)
	{
		m_cb.process_events();

		std::this_thread::sleep_for(10ms);
	}

	LOG_NOTICE(GENERAL, "All threads stopped...");

	idm::clear();
	fxm::clear();

	LOG_NOTICE(GENERAL, "Objects cleared...");

	finalize_psv_modules();

	for (auto& v : decltype(g_armv7_dump)(std::move(g_armv7_dump)))
	{
		LOG_NOTICE(ARMv7, "%s", v.second);
	}

	m_rsx_callback = 0;
	m_cpu_thr_stop = 0;

	// Temporary hack
	g_cfg_vfs_app_home = "";
	g_cfg_vfs_dev_bdvd = "";

	// TODO: check finalization order

	SavePoints(fs::get_config_dir() + BreakPointsDBName);
	m_break_points.clear();
	m_marked_points.clear();

	GetCallbackManager().Clear();
	GetModuleManager().Close();

	RSXIOMem.Clear();
	vm::close();

	SendDbgCommand(DID_STOPPED_EMU);

	if (g_cfg_autoexit)
	{
		GetCallbacks().exit();
	}
}

void Emulator::SavePoints(const std::string& path)
{
	const u32 break_count = size32(m_break_points);
	const u32 marked_count = size32(m_marked_points);

	fs::file(path, fom::rewrite)
		.write(bpdb_version)
		.write(break_count)
		.write(marked_count)
		.write(m_break_points)
		.write(m_marked_points);
}

bool Emulator::LoadPoints(const std::string& path)
{
	if (fs::file f{ path })
	{
		u16 version;
		u32 break_count;
		u32 marked_count;

		if (!f.read(version) || !f.read(break_count) || !f.read(marked_count))
		{
			LOG_ERROR(LOADER, "BP file '%s' is broken (length=0x%llx)", path, f.size());
			return false;
		}

		if (version != bpdb_version)
		{
			LOG_ERROR(LOADER, "BP file '%s' has unsupported version (version=0x%x)", path, version);
			return false;
		}

		m_break_points.resize(break_count);
		m_marked_points.resize(marked_count);

		if (!f.read(m_break_points) || !f.read(m_marked_points))
		{
			LOG_ERROR(LOADER, "'BP file %s' is broken (length=0x%llx, break_count=%u, marked_count=%u)", path, f.size(), break_count, marked_count);
			return false;
		}

		return true;
	}

	return false;
}

Emulator Emu;
