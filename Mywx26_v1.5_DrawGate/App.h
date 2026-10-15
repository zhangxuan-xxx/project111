#pragma once
#include <wx/wx.h>
#include "cMain.h"
class cApp :public wxApp
{
public:
	cMain* m_frame1;

private:


public:
	virtual bool OnInit();
};


