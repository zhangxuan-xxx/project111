
#pragma once

#include <wx/wx.h>
#include <wx/dcgraph.h>
#include <wx/dcbuffer.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <vector>
#include <algorithm>
#include "compoent.h"

// 工具被取消 / 切换时通知主窗口（用于刷新状态栏）
wxDECLARE_EVENT(EVT_DRAWBOARD_TOOL_CHANGED, wxCommandEvent);

class DrawBoard : public wxPanel
{
public:
    wxPoint m_mousePos;
    wxPoint m_wireStartPos;
    wxPoint m_wireEndPos;

    DrawBoard(wxFrame* parent);
    ~DrawBoard();

    void SetCurrentTool(ToolType tool)
    {
        m_currentTool = tool;
    }

    // -------- 网格显示 / 大小 / 吸附 --------
    bool IsGridVisible() const
    {
        return m_gridVisible;
    }

    void SetGridVisible(bool visible);

    int GetGridSize() const
    {
        return m_gridSize;
    }

    void SetGridSize(int size);

    bool IsSnapToGrid() const
    {
        return m_snapToGrid;
    }

    void SetSnapToGrid(bool snap);

    // 清空画布（新建）
    void ClearAll();

    // 取消当前操作并回到选择工具（Esc）
    void CancelCurrentAction();

    bool isDrawingAndGate = false;
    bool isDrawingOrGate = false;
    bool isDrawingNotGate = false;
    bool isDrawingNandGate = false;
    bool isDrawingXorGate = false;
    bool isDrawingNorGate = false;
    bool isDrawingXnorGate = false;

    // 供主窗口读取当前选中对象（属性对话框用）
    Gate* GetSelectedGate() const
    {
        return m_selectedGate;
    }

    int GetSelectedTextIndex() const
    {
        return m_selectedText;
    }

    void EditSelectedText()
    {
        EditText(m_selectedText);
    }

    // 画布上的文字标注
    struct BoardText
    {
        wxPoint pos;
        wxString text;
    };

    const std::vector<BoardText>& GetTexts() const
    {
        return m_texts;
    }

    void DeleteSelectedComponent()//增加公开删除接口
    {
        if (m_selectedText >= 0)
        {
            DeleteSelectedText();
            return;
        }

        DeleteSelectedGate();
    }
private:
    // 当前工具
    ToolType m_currentTool = TOOL_SELECT;

    // 连线状态
    bool m_wiring = false;
    bool m_wireDragging = false;
    wxPoint m_wirePressPos;
    int m_wireStartComponentId = -1;
    std::vector<Wire> m_wires;//一个容器，记录线的起点终点，还有所连接元件的ID
    int m_selectedWire = -1;

    // 文字标注与选中状态
    std::vector<BoardText> m_texts;
    int m_selectedText = -1;

    // 画布上的逻辑门
    std::vector<Gate*> Gates;

    // 元件选择与拖动状态
    Gate* m_selectedGate = nullptr;
    // 元件 ID 从 1 开始，创建后不重复使用
    int m_nextGateId = 1;
    bool m_draggingGate = false;

    // 鼠标按下时，鼠标相对元件中心的偏移量
    wxPoint m_dragOffset;

    // 网格与吸附
    bool m_gridVisible = true;
    bool m_snapToGrid = true;
    int m_gridSize = 20;

    // 把坐标吸附到最近的网格交点
    wxPoint SnapPoint(const wxPoint& pos) const;

    // 通知主窗口工具状态变化（0=放置完成，1=被 Esc 取消）
    void NotifyToolChanged(int reason);

    // 绘图
    void OnPaint(wxPaintEvent& event);
    void drawGrid(wxGraphicsContext* gc);

    // 鼠标事件
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnRightClick(wxMouseEvent& event);

    // 文字标注
    int FindTextAt(const wxPoint& pos) const;
    void DeleteSelectedText();
    void EditText(int index);
    bool PromptForText(wxString& value, const wxString& initial);

    // 连线命中判断
    int FindWireAt(const wxPoint& pos) const;

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

    // 连线：起笔 / 落笔 / 取消（支持空白处自由连线）
    void StartWire(const wxPoint& pos);
    void CompleteWire(const wxPoint& pos);
    void CancelWire();
    // 删除指定元件关联的所有连线
    void DeleteWiresForGate(int gateId);
};
