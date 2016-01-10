#include "stdafx.h"
#include "stdafx_gui.h"

#include "Utilities/Registry.h"
#include "FrameBase.h"

FrameBase::FrameBase(wxWindow* parent, wxWindowID id, const wxString& frame_name, const std::string& ini_name, wxSize defsize, wxPoint defposition, long style)
	: wxFrame(parent, id, frame_name, defposition, defsize, style)
	, ini_name(ini_name.empty() ? fmt::ToUTF8(frame_name) : ini_name)
{
	LoadInfo();

	Bind(wxEVT_CLOSE_WINDOW, &FrameBase::OnClose, this);
	Bind(wxEVT_MOVE, &FrameBase::OnMove, this);
	Bind(wxEVT_SIZE, &FrameBase::OnResize, this);
}

void FrameBase::SetSizerAndFit(wxSizer* sizer, bool deleteOld, bool loadinfo)
{
	wxFrame::SetSizerAndFit(sizer, deleteOld);
	if (loadinfo) LoadInfo();
}

void FrameBase::LoadInfo()
{
	wxSize size;
	if (cfg::try_to_int_pair<int, 'x'>(&size, cfg::to_string("gui/size/" + ini_name), 0))
		SetSize(size);

	wxPoint pos;
	if (cfg::try_to_int_pair<int, ':'>(&pos, cfg::to_string("gui/pos/" + ini_name)))
		SetPosition(pos);
}

void FrameBase::OnMove(wxMoveEvent& event)
{
	const auto& pos = GetPosition();
	ASSERT(cfg::from_string("gui/pos/" + ini_name, fmt::format("%d:%d", pos.x, pos.y)));
	event.Skip();
}

void FrameBase::OnResize(wxSizeEvent& event)
{
	const auto& size = GetSize();
	const auto& pos = GetPosition();
	ASSERT(cfg::from_string("gui/size/" + ini_name, fmt::format("%dx%d", size.GetWidth(), size.GetHeight())));
	ASSERT(cfg::from_string("gui/pos/" + ini_name, fmt::format("%d:%d", pos.x, pos.y)));
	//if (m_is_skip_resize) event.Skip();
}

void FrameBase::OnClose(wxCloseEvent& event)
{
	cfg::save();
	event.Skip();
}
