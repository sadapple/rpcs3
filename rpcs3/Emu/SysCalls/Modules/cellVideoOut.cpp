#include "stdafx.h"
#include "Utilities/Registry.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellVideoOut.h"

extern Module<> cellSysutil;

const extern cfg::map_entry<u8> g_cfg_video_out_resolution("rsx/Resolution", "1280x720",
{
	{ "1920x1080", CELL_VIDEO_OUT_RESOLUTION_1080 },
	{ "1280x720", CELL_VIDEO_OUT_RESOLUTION_720 },
	{ "720x480", CELL_VIDEO_OUT_RESOLUTION_480 },
	{ "720x576", CELL_VIDEO_OUT_RESOLUTION_576 },
	{ "1600x1080", CELL_VIDEO_OUT_RESOLUTION_1600x1080 },
	{ "1440x1080", CELL_VIDEO_OUT_RESOLUTION_1440x1080 },
	{ "1280x1080", CELL_VIDEO_OUT_RESOLUTION_1280x1080 },
	{ "960x1080", CELL_VIDEO_OUT_RESOLUTION_960x1080 },
});

const extern cfg::map_entry<u8> g_cfg_video_out_aspect_ratio("rsx/Aspect ratio", "16x9",
{
	{ "4x3", CELL_VIDEO_OUT_ASPECT_4_3 },
	{ "16x9", CELL_VIDEO_OUT_ASPECT_16_9 },
});

const extern std::unordered_map<u8, size2i> g_video_out_resolution_map
{
	{ CELL_VIDEO_OUT_RESOLUTION_1080,      { 1920, 1080 } },
	{ CELL_VIDEO_OUT_RESOLUTION_720,       { 1280, 720 } },
	{ CELL_VIDEO_OUT_RESOLUTION_480,       { 720, 480 } },
	{ CELL_VIDEO_OUT_RESOLUTION_576,       { 720, 576 } },
	{ CELL_VIDEO_OUT_RESOLUTION_1600x1080, { 1600, 1080 } },
	{ CELL_VIDEO_OUT_RESOLUTION_1440x1080, { 1440, 1080 } },
	{ CELL_VIDEO_OUT_RESOLUTION_1280x1080, { 1280, 1080 } },
	{ CELL_VIDEO_OUT_RESOLUTION_960x1080,  { 960, 1080 } },
};

s32 cellVideoOutGetState(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutState> state)
{
	cellSysutil.trace("cellVideoOutGetState(videoOut=%d, deviceIndex=%d, state=*0x%x)", videoOut, deviceIndex, state);

	if (deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		state->state = CELL_VIDEO_OUT_OUTPUT_STATE_DISABLED;
		state->colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
		state->displayMode.resolutionId = g_cfg_video_out_resolution.get(); // TODO
		state->displayMode.scanMode = CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE;
		state->displayMode.conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
		state->displayMode.aspect = g_cfg_video_out_aspect_ratio.get(); // TODO
		state->displayMode.refreshRates = CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ;
		return CELL_OK;

	case CELL_VIDEO_OUT_SECONDARY:
		*state = { CELL_VIDEO_OUT_OUTPUT_STATE_DISABLED }; // ???
		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetResolution(u32 resolutionId, vm::ptr<CellVideoOutResolution> resolution)
{
	cellSysutil.trace("cellVideoOutGetResolution(resolutionId=%d, resolution=*0x%x)", resolutionId, resolution);

	switch (resolutionId)
	{
	case CELL_VIDEO_OUT_RESOLUTION_1080:
	case CELL_VIDEO_OUT_RESOLUTION_720:
	case CELL_VIDEO_OUT_RESOLUTION_480:
	case CELL_VIDEO_OUT_RESOLUTION_576:
	case CELL_VIDEO_OUT_RESOLUTION_1600x1080:
	case CELL_VIDEO_OUT_RESOLUTION_1440x1080:
	case CELL_VIDEO_OUT_RESOLUTION_1280x1080:
	case CELL_VIDEO_OUT_RESOLUTION_960x1080:
	{
		const size2i size = g_video_out_resolution_map.at(static_cast<u8>(resolutionId));
		*resolution = { static_cast<u16>(size.width), static_cast<u16>(size.height) };
		break;
	}

	case CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING:
	case CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING:
	case CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING:
	case CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING:
	case CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING:
	case CELL_VIDEO_OUT_RESOLUTION_720_SIMULVIEW_FRAME_PACKING:
	{
		throw EXCEPTION("Unexpected resolutionId (%u)", resolutionId);
	}
		
	default: return CELL_VIDEO_OUT_ERROR_ILLEGAL_PARAMETER;
	}

	return CELL_OK;
}

s32 cellVideoOutConfigure(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option, u32 waitForEvent)
{
	cellSysutil.warning("cellVideoOutConfigure(videoOut=%d, config=*0x%x, option=*0x%x, waitForEvent=%d)", videoOut, config, option, waitForEvent);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		// TODO
		return CELL_OK;

	case CELL_VIDEO_OUT_SECONDARY:
		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetConfiguration(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option)
{
	cellSysutil.warning("cellVideoOutGetConfiguration(videoOut=%d, config=*0x%x, option=*0x%x)", videoOut, config, option);

	if (option) *option = {};
	*config = {};

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY:
		config->resolutionId = g_cfg_video_out_resolution.get();
		config->format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
		config->aspect = g_cfg_video_out_aspect_ratio.get();
		config->pitch = 4 * g_video_out_resolution_map.at(g_cfg_video_out_resolution.get()).width;

		return CELL_OK;

	case CELL_VIDEO_OUT_SECONDARY:

		return CELL_OK;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetDeviceInfo(u32 videoOut, u32 deviceIndex, vm::ptr<CellVideoOutDeviceInfo> info)
{
	cellSysutil.warning("cellVideoOutGetDeviceInfo(videoOut=%d, deviceIndex=%d, info=*0x%x)", videoOut, deviceIndex, info);

	if (deviceIndex) return CELL_VIDEO_OUT_ERROR_DEVICE_NOT_FOUND;

	// Use standard dummy values for now.
	info->portType = CELL_VIDEO_OUT_PORT_HDMI;
	info->colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
	info->latency = 1000;
	info->availableModeCount = 1;
	info->state = CELL_VIDEO_OUT_DEVICE_STATE_AVAILABLE;
	info->rgbOutputRange = 1;
	info->colorInfo.blueX = 0xFFFF;
	info->colorInfo.blueY = 0xFFFF;
	info->colorInfo.greenX = 0xFFFF;
	info->colorInfo.greenY = 0xFFFF;
	info->colorInfo.redX = 0xFFFF;
	info->colorInfo.redY = 0xFFFF;
	info->colorInfo.whiteX = 0xFFFF;
	info->colorInfo.whiteY = 0xFFFF;
	info->colorInfo.gamma = 100;
	info->availableModes[0].aspect = 0;
	info->availableModes[0].conversion = 0;
	info->availableModes[0].refreshRates = 0xF;
	info->availableModes[0].resolutionId = 1;
	info->availableModes[0].scanMode = 0;

	return CELL_OK;
}

s32 cellVideoOutGetNumberOfDevice(u32 videoOut)
{
	cellSysutil.warning("cellVideoOutGetNumberOfDevice(videoOut=%d)", videoOut);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return 1;
	case CELL_VIDEO_OUT_SECONDARY: return 0;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetResolutionAvailability(u32 videoOut, u32 resolutionId, u32 aspect, u32 option)
{
	cellSysutil.warning("cellVideoOutGetResolutionAvailability(videoOut=%d, resolutionId=0x%x, aspect=%d, option=%d)", videoOut, resolutionId, aspect, option);

	switch (videoOut)
	{
	case CELL_VIDEO_OUT_PRIMARY: return 1;
	case CELL_VIDEO_OUT_SECONDARY: return 0;
	}

	return CELL_VIDEO_OUT_ERROR_UNSUPPORTED_VIDEO_OUT;
}

s32 cellVideoOutGetConvertCursorColorInfo(vm::ptr<u8> rgbOutputRange)
{
	throw EXCEPTION("");
}

s32 cellVideoOutDebugSetMonitorType(u32 videoOut, u32 monitorType)
{
	throw EXCEPTION("");
}

s32 cellVideoOutRegisterCallback(u32 slot, vm::ptr<CellVideoOutCallback> function, vm::ptr<void> userData)
{
	throw EXCEPTION("");
}

s32 cellVideoOutUnregisterCallback(u32 slot)
{
	throw EXCEPTION("");
}


void cellSysutil_VideoOut_init()
{
	REG_FUNC(cellSysutil, cellVideoOutGetState);
	REG_FUNC(cellSysutil, cellVideoOutGetResolution);
	REG_FUNC(cellSysutil, cellVideoOutConfigure);
	REG_FUNC(cellSysutil, cellVideoOutGetConfiguration);
	REG_FUNC(cellSysutil, cellVideoOutGetDeviceInfo);
	REG_FUNC(cellSysutil, cellVideoOutGetNumberOfDevice);
	REG_FUNC(cellSysutil, cellVideoOutGetResolutionAvailability);
	REG_FUNC(cellSysutil, cellVideoOutGetConvertCursorColorInfo);
	REG_FUNC(cellSysutil, cellVideoOutDebugSetMonitorType);
	REG_FUNC(cellSysutil, cellVideoOutRegisterCallback);
	REG_FUNC(cellSysutil, cellVideoOutUnregisterCallback);
}
