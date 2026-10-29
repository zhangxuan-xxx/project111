#include "IconFactory.h"

#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/graphics.h>
#include <wx/image.h>

#include <cmath>
#include <cstring>
#include <vector>

// ============================================================
// 所有图标在 24x24 的逻辑坐标里画，最后按 size 等比缩放
// ============================================================
namespace
{
    const wxColour kInk(56, 61, 68);
    const double kStroke = 1.7;
    const double kPi = 3.14159265358979323846;

    using Pts = std::vector<wxPoint2DDouble>;

    void Polyline(wxGraphicsContext& gc, const Pts& pts)
    {
        if (pts.size() >= 2)
        {
            gc.StrokeLines(pts.size(), pts.data());
        }
    }

    void Rect(wxGraphicsContext& gc,
              double x, double y, double w, double h)
    {
        Pts pts = {
            {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}, {x, y}
        };
        Polyline(gc, pts);
    }

    void Dot(wxGraphicsContext& gc, double x, double y, double r)
    {
        gc.SetBrush(wxBrush(kInk));
        gc.DrawEllipse(x - r, y - r, 2 * r, 2 * r);
        gc.SetBrush(*wxTRANSPARENT_BRUSH);
    }

    // 带箭头的直线，箭头夹角约 40 度
    void Arrow(wxGraphicsContext& gc,
               double x1, double y1, double x2, double y2,
               double head = 3.4)
    {
        gc.StrokeLine(x1, y1, x2, y2);

        const double a = std::atan2(y2 - y1, x2 - x1);

        for (double d : { 0.62, -0.62 })
        {
            gc.StrokeLine(
                x2, y2,
                x2 - head * std::cos(a + d),
                y2 - head * std::sin(a + d));
        }
    }

    // 半圆弧（用于与门符号的右侧）
    void RightArc(wxGraphicsContext& gc,
                  double cx, double cy, double r)
    {
        Pts pts;
        for (int deg = -90; deg <= 90; deg += 10)
        {
            const double t = deg * kPi / 180.0;
            pts.emplace_back(cx + r * std::cos(t), cy + r * std::sin(t));
        }
        Polyline(gc, pts);
    }

    void DrawIcon(wxGraphicsContext& gc, IconId id)
    {
        switch (id)
        {
        case IconId::New:
            Rect(gc, 6, 3, 12, 18);
            Polyline(gc, { {12, 3}, {12, 8}, {18, 8} });
            break;

        case IconId::Open:
            Polyline(gc, { {3, 7}, {9, 7}, {11, 5},
                           {20, 5}, {20, 20}, {3, 20}, {3, 7} });
            break;

        case IconId::Save:
            Polyline(gc, { {4, 4}, {16, 4}, {20, 8},
                           {20, 20}, {4, 20}, {4, 4} });
            Rect(gc, 8, 13, 8, 7);
            Rect(gc, 8, 4, 8, 4);
            break;

        case IconId::Undo:
            Polyline(gc, { {14, 9}, {5, 9} });
            Polyline(gc, { {14, 9}, {17, 12}, {14, 15}, {9, 15} });
            Arrow(gc, 9, 9, 4, 9, 3.2);
            break;

        case IconId::Redo:
            Polyline(gc, { {10, 9}, {19, 9} });
            Polyline(gc, { {10, 9}, {7, 12}, {10, 15}, {15, 15} });
            Arrow(gc, 15, 9, 20, 9, 3.2);
            break;

        case IconId::Cut:
            gc.DrawEllipse(4, 6, 4.5, 4.5);
            gc.DrawEllipse(4, 14, 4.5, 4.5);
            gc.StrokeLine(8.2, 9.2, 18, 19);
            gc.StrokeLine(8.2, 15.8, 18, 6);
            break;

        case IconId::Copy:
            Polyline(gc, { {9, 4}, {20, 4}, {20, 18}, {15, 18} });
            Rect(gc, 4, 8, 11, 12);
            break;

        case IconId::Paste:
            Rect(gc, 6, 5, 12, 16);
            Polyline(gc, { {9, 5}, {9, 3}, {15, 3}, {15, 5} });
            break;

        case IconId::Delete:
            gc.StrokeLine(5, 7, 19, 7);
            Polyline(gc, { {10, 7}, {10, 4.5}, {14, 4.5}, {14, 7} });
            Polyline(gc, { {7, 7}, {8, 20}, {16, 20}, {17, 7} });
            gc.StrokeLine(10.2, 10, 10.5, 17);
            gc.StrokeLine(13.8, 10, 13.5, 17);
            break;

        case IconId::ZoomIn:
            gc.DrawEllipse(4, 4, 12, 12);
            gc.StrokeLine(14.6, 14.6, 20, 20);
            gc.StrokeLine(6.5, 10, 13.5, 10);
            gc.StrokeLine(10, 6.5, 10, 13.5);
            break;

        case IconId::ZoomOut:
            gc.DrawEllipse(4, 4, 12, 12);
            gc.StrokeLine(14.6, 14.6, 20, 20);
            gc.StrokeLine(6.5, 10, 13.5, 10);
            break;

        case IconId::Fit:
            Rect(gc, 5, 5, 14, 14);
            Arrow(gc, 12, 12, 5.5, 12, 3.0);
            Arrow(gc, 12, 12, 18.5, 12, 3.0);
            break;

        case IconId::Grid:
            for (int y : { 6, 12, 18 })
            {
                for (int x : { 6, 12, 18 })
                {
                    Dot(gc, static_cast<double>(x),
                        static_cast<double>(y), 1.4);
                }
            }
            break;

        case IconId::Select:
        {
            Pts pts = {
                {6, 3}, {6, 19}, {9.8, 15.2}, {12.4, 21},
                {15.2, 19.6}, {12.8, 13.8}, {18.5, 13.2}
            };
            gc.SetBrush(wxBrush(kInk));

            {
                wxGraphicsPath path = gc.CreatePath();
                path.MoveToPoint(pts[0]);

                for (size_t i = 1; i < pts.size(); ++i)
                {
                    path.AddLineToPoint(pts[i]);
                }

                path.CloseSubpath();
                gc.FillPath(path);
            }

            gc.SetBrush(*wxTRANSPARENT_BRUSH);
            break;
        }

        case IconId::Wire:
            Polyline(gc, { {4, 18}, {10, 18}, {10, 8}, {19, 8} });
            Dot(gc, 4, 18, 1.6);
            Dot(gc, 19, 8, 1.6);
            break;

        case IconId::Text:
            gc.StrokeLine(7, 19, 12, 5);
            gc.StrokeLine(12, 5, 17, 19);
            gc.StrokeLine(9, 14, 15, 14);
            break;

        case IconId::Gate:
            gc.StrokeLine(5, 6, 5, 18);
            gc.StrokeLine(5, 6, 12, 6);
            gc.StrokeLine(5, 18, 12, 18);
            RightArc(gc, 12, 12, 6);
            break;

        default:
            break;
        }
    }
}

wxBitmap MakeIcon(IconId id, int size)
{
    // 先建一张全透明的图，再在上面画线条
    wxImage image(size, size);
    image.SetAlpha();

    if (image.HasAlpha())
    {
        std::memset(image.GetAlpha(), 0,
                    static_cast<size_t>(size) * size);
    }

    wxBitmap bitmap(image);

    {
        wxMemoryDC memoryDC(bitmap);

        if (memoryDC.IsOk())
        {
            // 直接在内存 DC 上建 wxGraphicsContext，才能用缩放和抗锯齿
            wxGraphicsContext* gc =
                wxGraphicsContext::Create(memoryDC);

            if (gc != nullptr)
            {
                const double scale = size / 24.0;
                gc->Scale(scale, scale);
                gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
                gc->SetPen(wxPen(kInk, kStroke));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);

                DrawIcon(*gc, id);

                delete gc;
            }
        }

        memoryDC.SelectObject(wxNullBitmap);
    }

    return bitmap;
}
