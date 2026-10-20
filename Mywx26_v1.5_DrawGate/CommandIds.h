#pragma once

#include <wx/defs.h>

// ============================================================
// 菜单 / 工具栏 / 右键菜单 共用的命令 ID
// 独立成头文件，主窗口和画布都能引用，避免互相包含
// ============================================================
enum
{
    ID_CMD_BASE = wxID_HIGHEST + 100,

    // 文件
    ID_FILE_NEW,
    ID_FILE_OPEN,
    ID_FILE_SAVE,
    ID_FILE_SAVEAS,
    ID_FILE_EXPORT_PNG,
    ID_FILE_EXPORT_SVG,
    ID_FILE_EXPORT_TRUTH,
    ID_FILE_EXIT,

    // 编辑
    ID_EDIT_UNDO,
    ID_EDIT_REDO,
    ID_EDIT_CUT,
    ID_EDIT_COPY,
    ID_EDIT_PASTE,
    ID_EDIT_DELETE,
    ID_EDIT_SELECTALL,
    ID_EDIT_PROPERTIES,

    // 视图
    ID_VIEW_ZOOMIN,
    ID_VIEW_ZOOMOUT,
    ID_VIEW_FIT,
    ID_VIEW_ACTUAL,
    ID_VIEW_GRID,
    ID_VIEW_SNAP,
    ID_VIEW_GRIDSIZE_10,
    ID_VIEW_GRIDSIZE_20,
    ID_VIEW_GRIDSIZE_50,

    // 工具模式
    ID_TOOL_SELECT,

    // 放置
    ID_PLACE_AND,
    ID_PLACE_OR,
    ID_PLACE_NOT,
    ID_PLACE_NAND,
    ID_PLACE_XOR,
    ID_PLACE_NOR,
    ID_PLACE_XNOR,
    ID_PLACE_WIRE,
    ID_PLACE_TEXT,
    ID_PLACE_PIN_IN,
    ID_PLACE_PIN_OUT,
    ID_PLACE_CONST0,
    ID_PLACE_CONST1,

    // 设计
    ID_DESIGN_ROTATE,
    ID_DESIGN_ROTATE_CCW,
    ID_DESIGN_MIRROR_H,
    ID_DESIGN_MIRROR_V,
    ID_DESIGN_ALIGN_LEFT,
    ID_DESIGN_ALIGN_HCENTER,
    ID_DESIGN_ALIGN_RIGHT,
    ID_DESIGN_ALIGN_TOP,
    ID_DESIGN_ALIGN_VCENTER,
    ID_DESIGN_ALIGN_BOTTOM,
    ID_DESIGN_DIST_H,
    ID_DESIGN_DIST_V,
    ID_DESIGN_GROUP,
    ID_DESIGN_UNGROUP,

    // 工具
    ID_TOOLS_LIBRARY,
    ID_TOOLS_PREFS,
    ID_TOOLS_SHORTCUTS,

    // 帮助
    ID_HELP_MANUAL,
    ID_HELP_SHORTCUTS,
    ID_HELP_ABOUT,

    // 右键菜单专用
    ID_CTX_EDIT_TEXT,
    ID_CTX_DELETE_TEXT
};
