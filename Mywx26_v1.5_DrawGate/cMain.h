#pragma once
#include <wx/wx.h>
#include "DrawBoard.h"
#include "ComponentLibrary.h"


class cMain : public wxFrame
{
public:
	cMain();
	~cMain();

public:
	wxButton* m_AND = nullptr;
	wxButton* m_OR = nullptr;
	wxButton* m_Delete;
	wxButton* m_btn1 = nullptr;
	wxTextCtrl* m_txt1 = nullptr;
	wxListBox* m_list1 = nullptr;
	wxButton* drawLineButton;
	DrawBoard* drawborad;

	void OnButtoClicked(wxCommandEvent& evt);
	void OnEnableDrawAndGate(wxCommandEvent& evt);
	void OnEnableDrawOrGate(wxCommandEvent& evt);
	void OnDeleteButtonClick(wxCommandEvent& evt);
	//wxDECLARE_EVENT_TABLE();
	void OnLibraryItemDoubleClick(wxCommandEvent& evt);
private:
	void OnDrawLineButtonClick(wxCommandEvent& event);

};

