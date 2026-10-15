#pragma once
#include <wx/wx.h>
enum ToolType {
    TOOL_SELECT,
    TOOL_WIRE
};

class Wire {
public:
    wxPoint start;
    wxPoint end;
    int startComponentId;
    int endComponentId;
    int startPortIndex;
    int endPortIndex;
    bool isActive;

    Wire() : startComponentId(-1), endComponentId(-1),
        startPortIndex(-1), endPortIndex(-1), isActive(false) {
    }

    void Draw(wxDC& dc) const {
        dc.SetPen(wxPen(isActive ? wxColour(255, 0, 0) : wxColour(0, 128, 0), 2));
        dc.DrawLine(start, end);
    }
};

enum GateType {
	AND,
	OR,
	NOT
};

class Component {
public:
	wxPoint center;
	double scale;
	bool scaling = false;
	bool moving = false;
};

class Gate :public Component {
public:
	GateType type = AND;
	//vector<unique_ptr<wxPoint>> pins;
	wxPoint pout;
	Gate(wxPoint center) :Component()
	{
		this->center = center;
	};
	virtual ~Gate() {}
	virtual void drawSelf(wxMemoryDC& memDC) const = 0;
};

class ANDGate :public Gate {
public:

	ANDGate(wxPoint center) :Gate(center) {
		this->type = AND;
		this->scale = 1.0;
	}

	virtual void drawSelf(wxMemoryDC& memDC) const override {

		wxGraphicsContext* gc = wxGraphicsContext::Create(memDC);

		gc->SetPen(wxPen(wxColour(0, 0, 0), 3));
		if (gc) {
			wxGraphicsPath path = gc->CreatePath();
			path.MoveToPoint(center.x + 20 * scale, center.y - 20 * scale);
			path.AddLineToPoint(center.x - 20 * scale, center.y - 20 * scale);
			path.AddLineToPoint(center.x - 20 * scale, center.y + 20 * scale);
			path.AddLineToPoint(center.x + 20 * scale, center.y + 20 * scale);
			path.AddArc(center.x + 20 * scale, center.y, 20 * scale, M_PI / 2, -M_PI / 2, false);

			path.MoveToPoint(center.x - 40 * scale, center.y + 10 * scale);
			path.AddLineToPoint(center.x - 20 * scale, center.y + 10 * scale);
			path.MoveToPoint(center.x - 40 * scale, center.y - 10 * scale);
			path.AddLineToPoint(center.x - 20 * scale, center.y - 10 * scale);
			path.MoveToPoint(center.x + 40 * scale, center.y);
			path.AddLineToPoint(center.x + 60 * scale, center.y);

			gc->StrokePath(path);
		}
		delete gc;
	}
};

class ORGate :public Gate {
public:
	ORGate(wxPoint center) :Gate(center) {
		type = OR;
		scale = 1.0;
	}
	virtual void drawSelf(wxMemoryDC& memDC)  const override {

		wxGraphicsContext* gc = wxGraphicsContext::Create(memDC);
		gc->SetPen(wxPen(wxColour(0, 0, 0), 3));
		if (gc) {
			wxGraphicsPath path = gc->CreatePath();
			path.MoveToPoint(center.x, center.y - 20 * scale);
			path.AddLineToPoint(center.x - 20 * scale, center.y - 20 * scale);
			path.AddQuadCurveToPoint(center.x, center.y, center.x - 20 * scale, center.y + 20 * scale);
			path.AddLineToPoint(center.x - 10 * scale, center.y + 20 * scale);
			path.AddQuadCurveToPoint(center.x + 20 * scale, center.y + 20 * scale, center.x + 40 * scale, center.y);
			path.AddQuadCurveToPoint(center.x + 20 * scale, center.y - 20 * scale, center.x, center.y - 20 * scale);

			path.MoveToPoint(center.x - 40 * scale, center.y + 10 * scale);
			path.AddLineToPoint(center.x - 13 * scale, center.y + 10 * scale);
			path.MoveToPoint(center.x - 40 * scale, center.y - 10 * scale);
			path.AddLineToPoint(center.x - 13 * scale, center.y - 10 * scale);
			path.MoveToPoint(center.x + 40 * scale, center.y);
			path.AddLineToPoint(center.x + 60 * scale, center.y);

			gc->StrokePath(path);
		}
		delete gc;
	}
};