#include "cMain.h"
#include "DrawBoard.h"



cMain::cMain() : wxFrame(nullptr,wxID_ANY,"Hello wxWidgets!",wxPoint(30,30),wxSize(800,600))
{
	m_txt1 = new wxTextCtrl(this, wxID_ANY, "", wxPoint(10, 70), wxSize(100, 30));
	m_list1 = new wxListBox(this, wxID_ANY, wxPoint(10, 110), wxSize(100, 300));

	m_AND = new wxButton(this, wxID_ANY, "Draw And Gate", wxPoint(10, 10), wxSize(100, 50));
	m_OR = new wxButton(this, wxID_ANY, "Draw Or Gate", wxPoint(10, 10), wxSize(100, 50));
	m_AND->Bind(wxEVT_BUTTON, &cMain::OnEnableDrawAndGate, this);
	m_OR->Bind(wxEVT_BUTTON, &cMain::OnEnableDrawOrGate, this);

	wxInitAllImageHandlers();
	wxBitmap bitmapForDrawLine;
	bool ifok = bitmapForDrawLine.LoadFile("resource/image/drawLine.png", wxBITMAP_TYPE_PNG);
	wxImage imageForDrawLine = bitmapForDrawLine.ConvertToImage();
	imageForDrawLine = imageForDrawLine.Scale(24, 24, wxIMAGE_QUALITY_HIGH);
	bitmapForDrawLine = wxBitmap(imageForDrawLine);

	drawLineButton = new wxBitmapButton(this, wxID_ANY, bitmapForDrawLine, wxDefaultPosition, wxDefaultSize);
	drawLineButton->Bind(wxEVT_BUTTON, &cMain::OnDrawLineButtonClick, this);
	drawLineButton->SetBackgroundColour(wxColour(255, 255, 255));
	drawLineButton->SetToolTip("Draw line");



	wxBoxSizer* controlboard = new wxBoxSizer(wxVERTICAL);
	controlboard->Add(drawLineButton, 0, wxALL | wxEXPAND, 5);
	controlboard->Add(m_txt1, 0, wxALL | wxEXPAND, 5);
	controlboard->Add(m_list1, 0, wxALL | wxEXPAND, 5);
	controlboard->Add(m_AND, 0, wxALL | wxEXPAND, 5);
	controlboard->Add(m_OR, 0, wxALL | wxEXPAND, 5);

	drawborad = new DrawBoard(this);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(controlboard, 0, wxALL | wxEXPAND, 5);
	mainSizer->Add(drawborad, 1, wxALL | wxEXPAND, 5);

	SetSizer(mainSizer);
}

cMain::~cMain() {
	
}

void cMain::OnEnableDrawAndGate(wxCommandEvent& evt) {
	drawborad->isDrawingAndGate = !drawborad->isDrawingAndGate;
	drawborad->isDrawingOrGate = false;
}

void cMain::OnEnableDrawOrGate(wxCommandEvent& evt) {
	drawborad->isDrawingOrGate = !drawborad->isDrawingOrGate;
	drawborad->isDrawingAndGate = false;
}

void cMain::OnButtoClicked(wxCommandEvent& evt) {
	m_list1->AppendString(m_txt1->GetValue());
	evt.Skip();
}

void cMain::OnDrawLineButtonClick(wxCommandEvent& event) {
	drawborad->SetCurrentTool(TOOL_WIRE);
}