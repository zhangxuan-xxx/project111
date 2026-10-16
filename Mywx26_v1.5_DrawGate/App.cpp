#include "App.h"
#include "cMain.h"

wxIMPLEMENT_APP(cApp);

bool cApp::OnInit()
{
	m_frame1 = new cMain();
	m_frame1->Show();
	return true;
}

