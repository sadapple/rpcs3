#include "stdafx.h"
#include "stdafx_gui.h"
#include "Utilities/Registry.h"
#include "Emu/System.h"

#ifdef _MSC_VER
#include <Windows.h>
#undef GetHwnd
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#endif

#include "SettingsDialog.h"

// Connects wx gui element to the property value; abstract base class
struct property_adapter
{
	const std::string name;

	property_adapter(const std::string& name)
		: name(name)
	{
	}

	virtual void save() = 0;
};

struct radiobox_pad_helper
{
	const std::string name;
	wxArrayString values;

	radiobox_pad_helper(const std::string& name)
		: name(name)
	{
		for (const auto& v : cfg::get_values(name))
		{
			values.Add(fmt::FromUTF8(v));
		}
	}

	operator const wxArrayString&() const
	{
		return values;
	}
};

struct radiobox_pad : property_adapter
{
	wxRadioBox*& ptr;

	radiobox_pad(const radiobox_pad_helper& helper, wxRadioBox*& ptr)
		: property_adapter(helper.name)
		, ptr(ptr)
	{
		ptr->SetSelection(static_cast<int>(cfg::to_index(name)));
	}

	void save() override
	{
		ASSERT(cfg::from_index(name, ptr->GetSelection()));
	}
};

struct combobox_pad : property_adapter
{
	wxComboBox*& ptr;

	combobox_pad(const std::string& name, wxComboBox*& ptr)
		: property_adapter(name)
		, ptr(ptr)
	{
		for (const auto& v : cfg::get_values(name))
		{
			ptr->Append(fmt::FromUTF8(v));
		}

		//ptr->SetSelection(static_cast<int>(cfg::to_index(name)));
		ptr->SetValue(fmt::FromUTF8(cfg::to_string(name)));
	}

	void save() override
	{
		//ASSERT(cfg::from_index(name, ptr->GetSelection()));
		ASSERT(cfg::from_string(name, fmt::ToUTF8(ptr->GetValue())));
	}
};

struct checkbox_pad : property_adapter
{
	wxCheckBox*& ptr;

	checkbox_pad(const std::string& name, wxCheckBox*& ptr)
		: property_adapter(name)
		, ptr(ptr)
	{
		ptr->SetValue(cfg::to_bool(name));
	}

	void save() override
	{
		ASSERT(cfg::from_bool(name, ptr->GetValue()));
	}
};

struct textctrl_pad : property_adapter
{
	wxTextCtrl*& ptr;

	textctrl_pad(const std::string& name, wxTextCtrl*& ptr)
		: property_adapter(name)
		, ptr(ptr)
	{
		ptr->SetValue(fmt::FromUTF8(cfg::to_string(name)));
	}

	void save() override
	{
		ASSERT(cfg::from_string(name, fmt::ToUTF8(ptr->GetValue())));
	}
};


SettingsDialog::SettingsDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition)
{
	std::vector<std::unique_ptr<property_adapter>> pads;

	const bool was_running = Emu.Pause();

	static const u32 width = 458;
	static const u32 height = 400;

	// Settings panels
	wxNotebook* nb_config = new wxNotebook(this, wxID_ANY, wxPoint(6, 6), wxSize(width, height));
	wxPanel* p_system = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_core = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_graphics = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_audio = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_io = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_misc = new wxPanel(nb_config, wxID_ANY);
	wxPanel* p_networking = new wxPanel(nb_config, wxID_ANY);

	nb_config->AddPage(p_core, wxT("Core"));
	nb_config->AddPage(p_graphics, wxT("Graphics"));
	nb_config->AddPage(p_audio, wxT("Audio"));
	nb_config->AddPage(p_io, wxT("Input / Output"));
	nb_config->AddPage(p_misc, wxT("Miscellaneous"));
	nb_config->AddPage(p_networking, wxT("Networking"));
	nb_config->AddPage(p_system, wxT("System"));

	wxBoxSizer* s_subpanel_core = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel_core1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_core2 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_graphics = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel_graphics1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_graphics2 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_audio = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_io = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* s_subpanel_io1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_io2 = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* s_subpanel_system = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_misc = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* s_subpanel_networking = new wxBoxSizer(wxVERTICAL);

	// Core settings
	wxStaticBoxSizer* s_round_llvm = new wxStaticBoxSizer(wxVERTICAL, p_core, _("LLVM config"));
	wxStaticBoxSizer* s_round_llvm_range = new wxStaticBoxSizer(wxHORIZONTAL, p_core, _("Excluded block range"));
	wxStaticBoxSizer* s_round_llvm_threshold = new wxStaticBoxSizer(wxHORIZONTAL, p_core, _("Compilation threshold"));

	// Graphics
	wxStaticBoxSizer* s_round_gs_render = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Render"));
	wxStaticBoxSizer* s_round_gs_d3d_adaptater = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("D3D Adaptater"));
	wxStaticBoxSizer* s_round_gs_res = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Resolution"));
	wxStaticBoxSizer* s_round_gs_aspect = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Aspect ratio"));
	wxStaticBoxSizer* s_round_gs_frame_limit = new wxStaticBoxSizer(wxVERTICAL, p_graphics, _("Frame limit"));

	// Input / Output
	wxStaticBoxSizer* s_round_io_pad_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Pad Handler"));
	wxStaticBoxSizer* s_round_io_keyboard_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Keyboard Handler"));
	wxStaticBoxSizer* s_round_io_mouse_handler = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Mouse Handler"));
	wxStaticBoxSizer* s_round_io_camera = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Camera"));
	wxStaticBoxSizer* s_round_io_camera_type = new wxStaticBoxSizer(wxVERTICAL, p_io, _("Camera type"));

	// Audio
	wxStaticBoxSizer* s_round_audio_out = new wxStaticBoxSizer(wxVERTICAL, p_audio, _("Audio Out"));

	// Miscellaneous
	wxStaticBoxSizer* s_round_hle_log_lvl = new wxStaticBoxSizer(wxVERTICAL, p_misc, _("Log Level"));

	// Networking
	wxStaticBoxSizer* s_round_net_status = new wxStaticBoxSizer(wxVERTICAL, p_networking, _("Connection status"));
	wxStaticBoxSizer* s_round_net_interface = new wxStaticBoxSizer(wxVERTICAL, p_networking, _("Network adapter"));

	// System
	wxStaticBoxSizer* s_round_sys_lang = new wxStaticBoxSizer(wxVERTICAL, p_system, _("Language"));


	wxRadioBox* rbox_ppu_decoder;
	wxRadioBox* rbox_spu_decoder;
	wxComboBox* cbox_gs_render = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_d3d_adaptater = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_resolution = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_aspect = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_gs_frame_limit = new wxComboBox(p_graphics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_pad_handler = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);;
	wxComboBox* cbox_keyboard_handler = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_mouse_handler = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_camera = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_camera_type = new wxComboBox(p_io, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_audio_out = new wxComboBox(p_audio, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_hle_loglvl = new wxComboBox(p_misc, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1), 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_net_status = new wxComboBox(p_networking, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	wxComboBox* cbox_sys_lang = new wxComboBox(p_system, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);

	wxCheckBox* chbox_core_llvm_exclud = new wxCheckBox(p_core, wxID_ANY, "Compiled blocks exclusion");
	wxCheckBox* chbox_core_hook_stfunc = new wxCheckBox(p_core, wxID_ANY, "Hook static functions");
	wxCheckBox* chbox_core_load_liblv2 = new wxCheckBox(p_core, wxID_ANY, "Load liblv2.sprx");
	wxCheckBox* chbox_gs_log_prog = new wxCheckBox(p_graphics, wxID_ANY, "Log shader programs");
	wxCheckBox* chbox_gs_dump_depth = new wxCheckBox(p_graphics, wxID_ANY, "Write Depth Buffer");
	wxCheckBox* chbox_gs_dump_color = new wxCheckBox(p_graphics, wxID_ANY, "Write Color Buffers");
	wxCheckBox* chbox_gs_read_color = new wxCheckBox(p_graphics, wxID_ANY, "Read Color Buffers");
	wxCheckBox* chbox_gs_read_depth = new wxCheckBox(p_graphics, wxID_ANY, "Read Depth Buffer");
	wxCheckBox* chbox_gs_vsync = new wxCheckBox(p_graphics, wxID_ANY, "VSync");
	wxCheckBox* chbox_gs_debug_output = new wxCheckBox(p_graphics, wxID_ANY, "Debug Output");
	wxCheckBox* chbox_gs_3dmonitor = new wxCheckBox(p_graphics, wxID_ANY, "3D Monitor");
	wxCheckBox* chbox_gs_overlay = new wxCheckBox(p_graphics, wxID_ANY, "Debug overlay");
	wxCheckBox* chbox_audio_dump = new wxCheckBox(p_audio, wxID_ANY, "Dump to file");
	wxCheckBox* chbox_audio_conv = new wxCheckBox(p_audio, wxID_ANY, "Convert to 16 bit");
	wxCheckBox* chbox_hle_exitonstop = new wxCheckBox(p_misc, wxID_ANY, "Exit RPCS3 when process finishes");
	wxCheckBox* chbox_hle_always_start = new wxCheckBox(p_misc, wxID_ANY, "Always start after boot");

	wxTextCtrl* txt_dbg_range_min = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(55, 20));
	wxTextCtrl* txt_dbg_range_max = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(55, 20));
	wxTextCtrl* txt_llvm_threshold = new wxTextCtrl(p_core, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(55, 20));

	//Auto Pause
	wxCheckBox* chbox_dbg_ap_systemcall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at System Call");
	wxCheckBox* chbox_dbg_ap_functioncall = new wxCheckBox(p_misc, wxID_ANY, "Auto Pause at Function Call");

	radiobox_pad_helper ppu_decoder_modes("core/PPU Decoder");
	rbox_ppu_decoder = new wxRadioBox(p_core, wxID_ANY, "PPU Decoder", wxDefaultPosition, wxSize(215, -1), ppu_decoder_modes, 1);
	pads.emplace_back(std::make_unique<radiobox_pad>(ppu_decoder_modes, rbox_ppu_decoder));

	radiobox_pad_helper spu_decoder_modes("core/SPU Decoder");
	rbox_spu_decoder = new wxRadioBox(p_core, wxID_ANY, "SPU Decoder", wxDefaultPosition, wxSize(215, -1), spu_decoder_modes, 1);
	pads.emplace_back(std::make_unique<radiobox_pad>(spu_decoder_modes, rbox_spu_decoder));

#ifdef _MSC_VER
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;

	if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&dxgi_factory))))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;

		for (UINT id = 0; dxgi_factory->EnumAdapters(id, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; id++)
		{
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);
			cbox_gs_d3d_adaptater->Append(desc.Description);
		}
	}
	else
#endif
	{
		cbox_gs_d3d_adaptater->Enable(false);
	}

	pads.emplace_back(std::make_unique<combobox_pad>("rsx/Resolution", cbox_gs_resolution));
	pads.emplace_back(std::make_unique<combobox_pad>("rsx/Aspect ratio", cbox_gs_aspect));
	pads.emplace_back(std::make_unique<combobox_pad>("rsx/Frame limit", cbox_gs_frame_limit));
	pads.emplace_back(std::make_unique<combobox_pad>("rsx/Renderer", cbox_gs_render));

	pads.emplace_back(std::make_unique<combobox_pad>("io/Pad Handler", cbox_pad_handler));
	pads.emplace_back(std::make_unique<combobox_pad>("io/Keyboard Handler", cbox_keyboard_handler));
	pads.emplace_back(std::make_unique<combobox_pad>("io/Mouse Handler", cbox_mouse_handler));
	pads.emplace_back(std::make_unique<combobox_pad>("audio/Audio Out", cbox_audio_out));

	pads.emplace_back(std::make_unique<combobox_pad>("io/Camera", cbox_camera));
	pads.emplace_back(std::make_unique<combobox_pad>("io/Camera type", cbox_camera_type));

	pads.emplace_back(std::make_unique<combobox_pad>("misc/log/Log Level", cbox_hle_loglvl));
	pads.emplace_back(std::make_unique<combobox_pad>("misc/net/Connection status", cbox_net_status));

	pads.emplace_back(std::make_unique<combobox_pad>("sys/Language", cbox_sys_lang));

	pads.emplace_back(std::make_unique<checkbox_pad>("core/llvm/Compiled blocks exclusion", chbox_core_llvm_exclud));
	pads.emplace_back(std::make_unique<checkbox_pad>("rsx/Log shader programs", chbox_gs_log_prog));
	pads.emplace_back(std::make_unique<checkbox_pad>("rsx/Write Depth Buffer", chbox_gs_dump_depth));
	pads.emplace_back(std::make_unique<checkbox_pad>("rsx/Write Color Buffers", chbox_gs_dump_color));
	pads.emplace_back(std::make_unique<checkbox_pad>("rsx/Read Color Buffers", chbox_gs_read_color));
	pads.emplace_back(std::make_unique<checkbox_pad>("rsx/Read Depth Buffer", chbox_gs_read_depth));
	pads.emplace_back(std::make_unique<checkbox_pad>("rsx/VSync", chbox_gs_vsync));
	pads.emplace_back(std::make_unique<checkbox_pad>("rsx/Debug Output", chbox_gs_debug_output));
	pads.emplace_back(std::make_unique<checkbox_pad>("rsx/3D Monitor", chbox_gs_3dmonitor));
	pads.emplace_back(std::make_unique<checkbox_pad>("rsx/Debug Overlay", chbox_gs_overlay));
	pads.emplace_back(std::make_unique<checkbox_pad>("audio/Dump to file", chbox_audio_dump));
	pads.emplace_back(std::make_unique<checkbox_pad>("audio/Convert to 16 bit", chbox_audio_conv));
	pads.emplace_back(std::make_unique<checkbox_pad>("misc/Exit RPCS3 when process finishes", chbox_hle_exitonstop));
	pads.emplace_back(std::make_unique<checkbox_pad>("misc/Always start after boot", chbox_hle_always_start));
	pads.emplace_back(std::make_unique<checkbox_pad>("core/Hook static functions", chbox_core_hook_stfunc));
	pads.emplace_back(std::make_unique<checkbox_pad>("core/Load liblv2.sprx", chbox_core_load_liblv2));

	//Auto Pause related
	pads.emplace_back(std::make_unique<checkbox_pad>("misc/debug/Auto Pause at System Call", chbox_dbg_ap_systemcall));
	pads.emplace_back(std::make_unique<checkbox_pad>("misc/debug/Auto Pause at Function Call", chbox_dbg_ap_functioncall));

	pads.emplace_back(std::make_unique<textctrl_pad>("core/llvm/Excluded block range min", txt_dbg_range_min));
	pads.emplace_back(std::make_unique<textctrl_pad>("core/llvm/Excluded block range max", txt_dbg_range_max));
	pads.emplace_back(std::make_unique<textctrl_pad>("core/llvm/Compilation threshold", txt_llvm_threshold));
	pads.emplace_back(std::make_unique<combobox_pad>("rsx/d3d12/D3D Adaptater", cbox_gs_d3d_adaptater));

	// Core
	s_round_llvm->Add(chbox_core_llvm_exclud, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm_range->Add(txt_dbg_range_min, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm_range->Add(txt_dbg_range_max, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm->Add(s_round_llvm_range, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm_threshold->Add(txt_llvm_threshold, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_llvm->Add(s_round_llvm_threshold, wxSizerFlags().Border(wxALL, 5).Expand());

	// Rendering
	s_round_gs_render->Add(cbox_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_d3d_adaptater->Add(cbox_gs_d3d_adaptater, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_res->Add(cbox_gs_resolution, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_aspect->Add(cbox_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_gs_frame_limit->Add(cbox_gs_frame_limit, wxSizerFlags().Border(wxALL, 5).Expand());

	// Input/Output
	s_round_io_pad_handler->Add(cbox_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_keyboard_handler->Add(cbox_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_mouse_handler->Add(cbox_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_camera->Add(cbox_camera, wxSizerFlags().Border(wxALL, 5).Expand());
	s_round_io_camera_type->Add(cbox_camera_type, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_audio_out->Add(cbox_audio_out, wxSizerFlags().Border(wxALL, 5).Expand());

	// Miscellaneous
	s_round_hle_log_lvl->Add(cbox_hle_loglvl, wxSizerFlags().Border(wxALL, 5).Expand());

	// Networking
	s_round_net_status->Add(cbox_net_status, wxSizerFlags().Border(wxALL, 5).Expand());

	s_round_sys_lang->Add(cbox_sys_lang, wxSizerFlags().Border(wxALL, 5).Expand());

	// Core
	s_subpanel_core1->Add(rbox_ppu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core2->Add(rbox_spu_decoder, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(s_round_llvm, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(chbox_core_hook_stfunc, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core1->Add(chbox_core_load_liblv2, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_core->Add(s_subpanel_core1);
	s_subpanel_core->Add(s_subpanel_core2);

	// Graphics
	s_subpanel_graphics1->Add(s_round_gs_render, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(s_round_gs_res, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(s_round_gs_d3d_adaptater, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_dump_color, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_read_color, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_dump_depth, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_read_depth, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics1->Add(chbox_gs_vsync, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(s_round_gs_aspect, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(s_round_gs_frame_limit, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->AddSpacer(68);
	s_subpanel_graphics2->Add(chbox_gs_debug_output, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_3dmonitor, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_overlay, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics2->Add(chbox_gs_log_prog, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_graphics->Add(s_subpanel_graphics1);
	s_subpanel_graphics->Add(s_subpanel_graphics2);

	// Input - Output
	s_subpanel_io1->Add(s_round_io_pad_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io1->Add(s_round_io_keyboard_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io1->Add(s_round_io_mouse_handler, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io2->Add(s_round_io_camera, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io2->Add(s_round_io_camera_type, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_io->Add(s_subpanel_io1);
	s_subpanel_io->Add(s_subpanel_io2);

	// Audio
	s_subpanel_audio->Add(s_round_audio_out, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_audio->Add(chbox_audio_dump, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_audio->Add(chbox_audio_conv, wxSizerFlags().Border(wxALL, 5).Expand());

	// Miscellaneous
	s_subpanel_misc->Add(s_round_hle_log_lvl, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_exitonstop, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_hle_always_start, wxSizerFlags().Border(wxALL, 5).Expand());

	// Auto Pause
	s_subpanel_misc->Add(chbox_dbg_ap_systemcall, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_misc->Add(chbox_dbg_ap_functioncall, wxSizerFlags().Border(wxALL, 5).Expand());

	// Networking
	s_subpanel_networking->Add(s_round_net_status, wxSizerFlags().Border(wxALL, 5).Expand());
	s_subpanel_networking->Add(s_round_net_interface, wxSizerFlags().Border(wxALL, 5).Expand());

	// System
	s_subpanel_system->Add(s_round_sys_lang, wxSizerFlags().Border(wxALL, 5).Expand());

	// Buttons
	wxBoxSizer* s_b_panel(new wxBoxSizer(wxHORIZONTAL));
	s_b_panel->Add(new wxButton(this, wxID_OK), wxSizerFlags().Border(wxALL, 5).Bottom());
	s_b_panel->Add(new wxButton(this, wxID_CANCEL), wxSizerFlags().Border(wxALL, 5).Bottom());

	// Resize panels 
	SetSizerAndFit(s_subpanel_core, false);
	SetSizerAndFit(s_subpanel_graphics, false);
	SetSizerAndFit(s_subpanel_io, false);
	SetSizerAndFit(s_subpanel_audio, false);
	SetSizerAndFit(s_subpanel_misc, false);
	SetSizerAndFit(s_subpanel_networking, false);
	SetSizerAndFit(s_subpanel_system, false);
	SetSizerAndFit(s_b_panel, false);

	SetSize(width + 26, height + 80);

	if (ShowModal() == wxID_OK)
	{
		for (auto& pad : pads)
		{
			pad->save();
		}
	}

	if (was_running) Emu.Resume();
}
