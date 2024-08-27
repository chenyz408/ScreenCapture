#include "ToolMain.h"
#include <format>
#include <string>
#include <ranges>
#include <vector>
#include "include/core/SkTextBlob.h"
#include "../CutMask.h"
#include "../WinMax.h"
#include "ToolSub.h"
#include "../Lang.h"
#include "../Screen.h"
#include "../Cmd.h"
#include "../App.h"

ToolMain::ToolMain()
{
}
ToolMain::~ToolMain()
{
}

void ToolMain::Init()
{
    for (size_t i = 0; i <= 14; i++)
    {
        Btns.push_back(ToolBtn(i));
    }
    listenLeftBtnDown(std::bind(&ToolMain::OnLeftBtnDown, this, std::placeholders::_1, std::placeholders::_2));
    listenLeftBtnUp(std::bind(&ToolMain::OnLeftBtnUp, this, std::placeholders::_1, std::placeholders::_2));
    listenMouseMove(std::bind(&ToolMain::OnMouseMove, this, std::placeholders::_1, std::placeholders::_2));
    listenMouseDrag(std::bind(&ToolMain::OnMouseDrag, this, std::placeholders::_1, std::placeholders::_2));
    listenCustomMsg(std::bind(&ToolMain::OnCustomMsg, this, std::placeholders::_1, std::placeholders::_2));
    listenPaint(std::bind(&ToolMain::OnPaint, this, std::placeholders::_1));
}

void ToolMain::OnCustomMsg(const EventType& type, const uint32_t& msg)
{
    if (type == EventType::maskReady) {
        setPositionByCutMask();
    }
    else if (type == EventType::undoDisable) {
        Btns[9].isDisable = msg;
    }
    else if (type == EventType::redoDisable) {
        Btns[10].isDisable = msg;
    }
}



void ToolMain::setPositionByCutMask()
{
    auto win = static_cast<WinMax*>(App::GetWin());
    auto mask = win->cutMask.get();
    float left{ mask->cutRect.fRight - Btns.size() * ToolBtn::Width };
    float top{ mask->cutRect.fBottom + MarginTop };
    //三个缝隙高度和两个工具条高度
    auto heightSpan = MarginTop * 3 + ToolBtn::Height * 2;
    auto screen = App::GetScreen(mask->cutRect.fRight, mask->cutRect.fBottom + heightSpan);
    if (screen) { //工具条右下角在屏幕内
        topFlag = false;
        //工具条左上角不在屏幕内
        if (!App::GetScreen(left, top)) {
            left = screen->fLeft;
        }
    }
    else { //工具条右下角不在屏幕内
        topFlag = true;
        //判断屏幕顶部是否有足够的空间，工具条是否可以在cutRect右上角
        screen = App::GetScreen(mask->cutRect.fRight, mask->cutRect.fTop - heightSpan);
        if (screen) { //工具条右上角在屏幕内  
            if (App::GetScreen(left, mask->cutRect.fTop - heightSpan)) { //工具条左上角在屏幕内
                top = mask->cutRect.fTop - MarginTop - ToolBtn::Height;
            }
            else { //工具条左上角不在屏幕中
                left = screen->fLeft;
                top = mask->cutRect.fTop - MarginTop - ToolBtn::Height;
            }
        }
        else { //工具条右上角不在屏幕内，此时屏幕顶部和屏幕底部都没有足够的空间，工具条只能显示在截图区域内            
            if (App::GetScreen(left, mask->cutRect.fBottom - heightSpan)) { //工具条左上角在屏幕内
                top = mask->cutRect.fBottom - MarginTop - ToolBtn::Height;
            }
            else { //工具条左上角不在屏幕中，得到截图区域所在屏幕
                screen = App::GetScreen(mask->cutRect.fRight, mask->cutRect.fBottom);
                if (screen) {
                    left = screen->fLeft;
                    top = mask->cutRect.fBottom - MarginTop - ToolBtn::Height;
                }
            }
        }
    }
    ToolRect.setXYWH(left, top, Btns.size() * ToolBtn::Width, ToolBtn::Height);
}
void ToolMain::SetPosition(const float& x, const float& y)
{
    ToolRect.setXYWH(x, y, Btns.size() * ToolBtn::Width, ToolBtn::Height);
}
void ToolMain::OnLeftBtnDown(const int& x, const int& y)
{
    isMouseDown = true;
    auto win = App::GetWin();
    if (win->state < State::tool)
    {
        return false;
    }
    if (!ToolRect.contains(x, y))
    {
        return false;
    }
    Recorder::Get()->ProcessText();
    win->IsMouseDown = false; //不然在主工具栏上拖拽的时候，会改变CutBox，而且改变完CutBox后不会在显示工具栏
    if (IndexHovered == IndexSelected)
    {
        Btns[IndexHovered]->IsSelected = false;
        IndexSelected = -1;
        win->state = State::tool;
        if (topFlag) {
            ToolRect.offset(0, MarginTop + ToolBtn::Height);
        }
        win->Refresh();
    }
    else
    {
        if (Btns[IndexHovered]->Selectable) {
            Btns[IndexHovered]->IsSelected = true;
            if (IndexSelected >= 0) {
                Btns[IndexSelected]->IsSelected = false;
            }
            else {
                if (topFlag) {
                    ToolRect.offset(0, 0 - MarginTop - ToolBtn::Height);
                }
            }
            IndexSelected = IndexHovered;
            ToolSub::Get()->InitBtns(IndexSelected);
            win->state = (State)(IndexSelected + 3);
            win->Refresh();
        }
        else {
            if (Btns[IndexHovered]->IsDisable) {
                return true;
            }
            switch (IndexHovered)
            {
            case 9: { //上一步
                Recorder::Get()->Undo();
                break;
            }
            case 10: { //下一步
                Recorder::Get()->Redo();
                break;
            }
            case 11: {
                App::Pin();
                break;
            }
            case 12: {
                App::SaveFile();
                Btns[12]->IsHover = false;
                break;
            }
            case 13: {
                App::GetWin()->SaveToClipboard();
                Btns[13]->IsHover = false;
                break;
            }
            case 14: {
                App::Quit(1);
                break;
            }
            default:
                break;
            }
        }
    }
    return true;
}

void ToolMain::OnPaint(SkCanvas* canvas)
{
    auto win = App::GetWin();
    if (win->state < State::tool)
    {
        return;
    }
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorWHITE);
    canvas->drawRect(ToolRect, paint);
    auto x = ToolRect.left();
    auto y = ToolRect.top();
    for (auto& btn : Btns)
    {
        btn.Paint(canvas, paint, x, y);
        x += ToolBtn::Width;
    }

    paint.setColor(0xFFB4B4B4);
    x = ToolRect.left() + ToolBtn::Width * 9;
    paint.setStroke(true);
    paint.setStrokeWidth(0.6f);
    paint.setStrokeCap(SkPaint::Cap::kRound_Cap);
    canvas->drawLine(SkPoint::Make(x, y + 12), SkPoint::Make(x, ToolRect.bottom() - 12), paint); //undo spliter

    x += ToolBtn::Width * 2;
    canvas->drawLine(SkPoint::Make(x, y + 12), SkPoint::Make(x, ToolRect.bottom() - 12), paint); //redo spliter
    paint.setColor(SkColorSetARGB(255, 22, 118, 255));
    canvas->drawRect(ToolRect, paint);
}

void ToolMain::SetUndoDisable(bool flag)
{
    Btns[9]->IsDisable = flag;
}

void ToolMain::SetRedoDisable(bool flag)
{
    Btns[10]->IsDisable = flag;
}


void ToolMain::UnSelectAndHoverAll()
{
    IndexSelected = -1;
    IndexHovered = -1;
    for (auto& btn : Btns) {
        btn->IsHover = false;
        btn->IsSelected = false;
    }
}


