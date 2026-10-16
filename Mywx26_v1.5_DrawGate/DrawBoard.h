
#pragma once

#include <wx/wx.h>
#include <wx/dcgraph.h>
#include <wx/dcbuffer.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <vector>
#include <algorithm>
#include "compoent.h"

class DrawBoard : public wxPanel
{
public:
    wxPoint m_mousePos;
    wxPoint m_wireStartPos;
    wxPoint m_wireEndPos;

    wxStaticText* st1;
    wxStaticText* st2;

    DrawBoard(wxFrame* parent);
    ~DrawBoard();

    void SetCurrentTool(ToolType tool)
    {
        m_currentTool = tool;
    }

    bool isDrawingAndGate = false;
    bool isDrawingOrGate = false;
    bool isDrawingNotGate = false;
    bool isDrawingNandGate = false;
    bool isDrawingXorGate = false;
    void DeleteSelectedComponent()//增加公开删除接口
    {
        DeleteSelectedGate();
    }
private:
    // 当前工具
    ToolType m_currentTool = TOOL_SELECT;

    // 连线状态
    bool m_wiring = false;
    int m_wireStartComponentId = -1;
    std::vector<Wire> m_wires;

    // 画布上的逻辑门
    std::vector<Gate*> Gates;

    // 元件选择与拖动状态
    Gate* m_selectedGate = nullptr;
    // 元件 ID 从 1 开始，创建后不重复使用
    int m_nextGateId = 1;
    bool m_draggingGate = false;

    // 鼠标按下时，鼠标相对元件中心的偏移量
    wxPoint m_dragOffset;

    // 绘图
    void OnPaint(wxPaintEvent& event);
    void drawGrid(wxGraphicsContext* gc);

    // 鼠标事件
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);

    // 元件编辑辅助函数
    Gate* FindGateAt(const wxPoint& pos);
    void DeleteSelectedGate();
    // 根据唯一 ID 查找元件
    Gate* FindGateById(int id);

    // 查找鼠标附近的输入端口
    bool FindInputPortAt(
        const wxPoint& pos,
        Gate*& gate,
        int& portIndex
    );

    // 查找鼠标附近的输出端口
    bool FindOutputPortAt(
        const wxPoint& pos,
        Gate*& gate
    );
    // 根据元件当前位置更新连接线端点
    void UpdateWirePositions();
    // 删除指定元件关联的所有连线
    void DeleteWiresForGate(int gateId);
};
