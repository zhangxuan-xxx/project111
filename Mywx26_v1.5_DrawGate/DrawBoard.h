#pragma once
#include <wx/wx.h>
#include <wx/dcgraph.h>
#include <wx/dcbuffer.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include "compoent.h"


class DrawBoard : public wxPanel
{
public:
	wxPoint m_mousePos;
	wxPoint m_wireStartPos;
	wxPoint m_wireEndPos;
	wxStaticText* st1;
	wxStaticText* st2;


public:
	DrawBoard(wxFrame* parent);
	~DrawBoard();

public:
	void SetCurrentTool(ToolType tool) { m_currentTool = tool; }
	bool isDrawingAndGate;
	bool isDrawingOrGate;

private:
	ToolType m_currentTool;
	bool m_wiring;
	std::vector<Wire> m_wires;

	std::vector<Gate*> Gates;


	void OnPaint(wxPaintEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnMouseDown(wxMouseEvent& event);
	void OnMouseUp(wxMouseEvent& event);

	void drawGrid(wxGraphicsContext* gc);

};
