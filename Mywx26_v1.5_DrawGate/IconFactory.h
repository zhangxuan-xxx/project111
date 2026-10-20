#pragma once

#include <wx/wx.h>

// ============================================================
// 工具栏图标：全部用矢量线条现画，不依赖外部图片资源
// 统一 24x24 坐标系，深灰线条，透明背景
// ============================================================
enum class IconId
{
    New,
    Open,
    Save,
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Delete,
    ZoomIn,
    ZoomOut,
    Fit,
    Grid,
    Select,
    Wire,
    Text,
    Gate
};

wxBitmap MakeIcon(IconId id, int size = 24);
