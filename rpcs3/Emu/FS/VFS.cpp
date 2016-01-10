#include "stdafx.h"
#include "Utilities/Registry.h"
#include "Emu/System.h"

#include "VFS.h"

cfg::string_entry g_cfg_vfs_emulator_dir("vfs/EmulatorDir", fs::get_executable_dir());
cfg::string_entry g_cfg_vfs_dev_hdd0("vfs/dev_hdd0", "$(EmulatorDir)dev_hdd0/");
cfg::string_entry g_cfg_vfs_dev_hdd1("vfs/dev_hdd1", "$(EmulatorDir)dev_hdd1/");
cfg::string_entry g_cfg_vfs_dev_flash("vfs/dev_flash", "$(EmulatorDir)dev_flash/");
cfg::string_entry g_cfg_vfs_dev_usb000("vfs/dev_usb000", "$(EmulatorDir)dev_usb000/");
cfg::string_entry g_cfg_vfs_dev_bdvd("vfs/dev_bdvd"); // Not mounted
cfg::string_entry g_cfg_vfs_app_home("vfs/app_home"); // Not mounted

const extern cfg::bool_entry g_cfg_vfs_allow_host_root("vfs/host_root", true);

std::string vfs::get(const std::string& vpath)
{
	// Temporary, very rough implementation
	const cfg::string_entry* vdir = nullptr;
	std::size_t start = 0;

	// Compare vpath with device name
	auto detect = [&](const auto& vdev) -> bool
	{
		const std::size_t size = ::size32(vdev) - 1; // Char array size

		if (vpath.compare(0, size, vdev, size) == 0)
		{
			start = size;
			return true;
		}

		return false;
	};

	if (g_cfg_vfs_allow_host_root && detect("/host_root/"))
		return vpath.substr(start); // Accessing host FS directly
	else if (detect("/dev_hdd0/"))
		vdir = &g_cfg_vfs_dev_hdd0;
	else if (detect("/dev_hdd1/"))
		vdir = &g_cfg_vfs_dev_hdd1;
	else if (detect("/dev_flash/"))
		vdir = &g_cfg_vfs_dev_flash;
	else if (detect("/dev_usb000/"))
		vdir = &g_cfg_vfs_dev_usb000;
	else if (detect("/dev_usb/"))
		vdir = &g_cfg_vfs_dev_usb000;
	else if (detect("/dev_bdvd/"))
		vdir = &g_cfg_vfs_dev_bdvd;
	else if (detect("/app_home/"))
		vdir = &g_cfg_vfs_app_home;

	// Return empty path if not mounted
	if (!vdir || !start)
	{
		LOG_WARNING(GENERAL, "vfs::get() failed for %s", vpath);
		return{};
	}

	// Replace path variables, concatenate
	return fmt::replace_all(*vdir, "$(EmulatorDir)", g_cfg_vfs_emulator_dir) + vpath.substr(start);
}
