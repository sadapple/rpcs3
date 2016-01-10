#include "stdafx.h"
#include "stdafx_gui.h"
#include "rpcs3.h"
#include "Utilities/Registry.h"
#include "KeyboardPadHandler.h"

cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_left_stick_left("io/kbpad/Left Analog Stick Left", 314);
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_left_stick_down("io/kbpad/Left Analog Stick Down", 317);
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_left_stick_right("io/kbpad/Left Analog Stick Right", 316);
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_left_stick_up("io/kbpad/Left Analog Stick Up", 315);
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_right_stick_left("io/kbpad/Right Analog Stick Left", 313);
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_right_stick_down("io/kbpad/Right Analog Stick Down", 367);
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_right_stick_right("io/kbpad/Right Analog Stick Right", 312);
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_right_stick_up("io/kbpad/Right Analog Stick Up", 366);
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_start("io/kbpad/Start", 13);
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_select("io/kbpad/Select", 32);
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_square("io/kbpad/Square", static_cast<int>('J'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_cross("io/kbpad/Cross", static_cast<int>('K'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_circle("io/kbpad/Circle", static_cast<int>('L'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_triangle("io/kbpad/Triangle", static_cast<int>('I'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_left("io/kbpad/Left", static_cast<int>('A'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_down("io/kbpad/Down", static_cast<int>('S'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_right("io/kbpad/Right", static_cast<int>('D'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_up("io/kbpad/Up", static_cast<int>('W'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_r1("io/kbpad/R1", static_cast<int>('3'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_r2("io/kbpad/R2", static_cast<int>('E'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_r3("io/kbpad/R3", static_cast<int>('C'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_l1("io/kbpad/L1", static_cast<int>('1'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_l2("io/kbpad/L2", static_cast<int>('Q'));
cfg::int_entry<INT_MIN, INT_MAX> g_cfg_io_kbpad_l3("io/kbpad/L3", static_cast<int>('Z'));

void KeyboardPadHandler::Init(const u32 max_connect)
{
	memset(&m_info, 0, sizeof(PadInfo));
	m_info.max_connect = max_connect;
	LoadSettings();
	m_info.now_connect = std::min(m_pads.size(), (size_t)max_connect);
}

KeyboardPadHandler::KeyboardPadHandler()
{
	wxGetApp().Bind(wxEVT_KEY_DOWN, &KeyboardPadHandler::KeyDown, this);
	wxGetApp().Bind(wxEVT_KEY_UP, &KeyboardPadHandler::KeyUp, this);
}

void KeyboardPadHandler::KeyDown(wxKeyEvent& event)
{
	Key(event.GetKeyCode(), 1);
	event.Skip();
}

void KeyboardPadHandler::KeyUp(wxKeyEvent& event)
{
	Key(event.GetKeyCode(), 0);
	event.Skip();
}

void KeyboardPadHandler::LoadSettings()
{
	//Fixed assign change, default is both sensor and press off
	m_pads.emplace_back(
		CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES,
		CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
		CELL_PAD_DEV_TYPE_STANDARD);

	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_cfg_io_kbpad_left, CELL_PAD_CTRL_LEFT);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_cfg_io_kbpad_down, CELL_PAD_CTRL_DOWN);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_cfg_io_kbpad_right, CELL_PAD_CTRL_RIGHT);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_cfg_io_kbpad_up, CELL_PAD_CTRL_UP);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_cfg_io_kbpad_start, CELL_PAD_CTRL_START);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_cfg_io_kbpad_r3, CELL_PAD_CTRL_R3);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_cfg_io_kbpad_l3, CELL_PAD_CTRL_L3);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_cfg_io_kbpad_select, CELL_PAD_CTRL_SELECT);

	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_cfg_io_kbpad_square, CELL_PAD_CTRL_SQUARE);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_cfg_io_kbpad_cross, CELL_PAD_CTRL_CROSS);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_cfg_io_kbpad_circle, CELL_PAD_CTRL_CIRCLE);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_cfg_io_kbpad_triangle, CELL_PAD_CTRL_TRIANGLE);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_cfg_io_kbpad_r1, CELL_PAD_CTRL_R1);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_cfg_io_kbpad_l1, CELL_PAD_CTRL_L1);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_cfg_io_kbpad_r2, CELL_PAD_CTRL_R2);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_cfg_io_kbpad_l2, CELL_PAD_CTRL_L2);

	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, g_cfg_io_kbpad_left_stick_left, g_cfg_io_kbpad_left_stick_right);
	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, g_cfg_io_kbpad_left_stick_up, g_cfg_io_kbpad_left_stick_down);
	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, g_cfg_io_kbpad_right_stick_left, g_cfg_io_kbpad_right_stick_right);
	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, g_cfg_io_kbpad_right_stick_up, g_cfg_io_kbpad_right_stick_down);
}
