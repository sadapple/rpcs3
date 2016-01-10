#pragma once

#include "Emu/Io/PadHandler.h"

class KeyboardPadHandler final : public PadHandlerBase, public wxWindow
{
public:
	virtual void Init(const u32 max_connect) override;

	KeyboardPadHandler();

	void KeyDown(wxKeyEvent& event);
	void KeyUp(wxKeyEvent& event);
	void LoadSettings();
};
