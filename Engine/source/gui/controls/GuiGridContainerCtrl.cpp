#include "GuiGridContainerCtrl.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDrawUtil.h"
#include "math/mMathFn.h"
#include "gui/core/guiControl.h"
#include "gui/core/guiCanvas.h"

IMPLEMENT_CONOBJECT(GuiGridContainerCtrl);

GuiGridContainerCtrl::GuiGridContainerCtrl()
    : mGridSpacing(16),
    mBgColor(0, 32, 0),
    mFgColor(0, 255, 0),
    mLineThickness(1),
    mDragging(false),
    mHovering(false),
    mShowCrosshair(false),
    mDottedGrid(false)
{
}

void GuiGridContainerCtrl::initPersistFields() {
    addGroup("Grid");
    addField("gridSpacing", TypeS32, Offset(mGridSpacing, GuiGridContainerCtrl));
    addField("bgColor", TypeColorI, Offset(mBgColor, GuiGridContainerCtrl));
    addField("fgColor", TypeColorI, Offset(mFgColor, GuiGridContainerCtrl));
    addField("lineThickness", TypeS32, Offset(mLineThickness, GuiGridContainerCtrl));
    addField("showCrosshair", TypeBool, Offset(mShowCrosshair, GuiGridContainerCtrl), "Toggle crosshair & bracket highlighting on hover/drag");
    addField("dottedGrid", TypeBool, Offset(mDottedGrid, GuiGridContainerCtrl), "Render interior grid lines as dotted instead of continuous");
    endGroup("Grid");
    Parent::initPersistFields();
}


Point2I GuiGridContainerCtrl::snapToGrid(const Point2I& pt) const {
    // snap to nearest grid intersection
    S32 x = (pt.x / mGridSpacing) * mGridSpacing;
    S32 y = (pt.y / mGridSpacing) * mGridSpacing;
    return Point2I(x, y);
}

void GuiGridContainerCtrl::onMouseDragged(const GuiEvent& event) {
    mDragging = true;
    Point2I local = globalToLocalCoord(Point2I(event.mousePoint.x, event.mousePoint.y));
    mCrossPos = local;
    if (mShowCrosshair)
    {
        setUpdate(); // Only if animation is needed
    }
    return Parent::onMouseDragged(event);
}

void GuiGridContainerCtrl::onMouseMove(const GuiEvent& event) {
    // show hover crosshair snapped to grid
    Point2I local = globalToLocalCoord(Point2I(event.mousePoint.x, event.mousePoint.y));
    mHovering = true;
    mCrossPos = local;
    if (mShowCrosshair)
    {
        setUpdate(); // Only if animation is needed
    }
    return Parent::onMouseMove(event);
}

void GuiGridContainerCtrl::onMouseEnter(const GuiEvent& event) {
    mHovering = true;
    // initialize at current mouse pos
    Point2I local = globalToLocalCoord(Point2I(event.mousePoint.x, event.mousePoint.y));
    mCrossPos = local;
    if (mShowCrosshair)
    {
        setUpdate(); // Only if animation is needed
    }
    Parent::onMouseEnter(event);
}

void GuiGridContainerCtrl::onMouseLeave(const GuiEvent& event) {
    mHovering = false;
    mDragging = false;
    if (mShowCrosshair)
    {
        setUpdate(); // Only if animation is needed
    }
    Parent::onMouseLeave(event);
}

void GuiGridContainerCtrl::onMouseUp(const GuiEvent& event) {
    mDragging = false;

    if (mShowCrosshair)
    {
        setUpdate(); // Only if animation is needed
    }

    Parent::onMouseUp(event);
}

//=============================================================================
// GuiGridContainerCtrl::onRender  –  dotted / solid grid + border
//=============================================================================
void GuiGridContainerCtrl::onRender(Point2I offset, const RectI& updateRect)
{
    const S32 W = getWidth();
    const S32 H = getHeight();

    // Background fill
    drawRectFill(offset, getExtent(), mBgColor);

    // Grid interior
    if (!mDottedGrid)
    {
        for (S32 x = mGridSpacing; x < W; x += mGridSpacing)
            for (S32 t = 0; t < mLineThickness; ++t)
                drawLine(offset + Point2I(x + t, 0), offset + Point2I(x + t, H), mFgColor);

        for (S32 y = mGridSpacing; y < H; y += mGridSpacing)
            for (S32 t = 0; t < mLineThickness; ++t)
                drawLine(offset + Point2I(0, y + t), offset + Point2I(W, y + t), mFgColor);
    }
    else
    {
        const S32 dot = mLineThickness, gap = dot * 2;
        for (S32 x = mGridSpacing; x < W; x += mGridSpacing)
            for (S32 y = 0; y < H; y += dot + gap)
                drawRectFill(offset + Point2I(x, y), Point2I(dot, dot), mFgColor);

        for (S32 y = mGridSpacing; y < H; y += mGridSpacing)
            for (S32 x = 0; x < W; x += dot + gap)
                drawRectFill(offset + Point2I(x, y), Point2I(dot, dot), mFgColor);
    }

    // Border
    if (!mDottedGrid)
    {
        drawRectFill(offset, Point2I(W, mLineThickness), mFgColor);
        drawRectFill(offset + Point2I(0, H - mLineThickness), Point2I(W, mLineThickness), mFgColor);
        drawRectFill(offset, Point2I(mLineThickness, H), mFgColor);
        drawRectFill(offset + Point2I(W - mLineThickness, 0), Point2I(mLineThickness, H), mFgColor);
    }
    else
    {
        const S32 d = mLineThickness, gap = d * 2;
        for (S32 x = 0; x < W; x += d + gap)
        {
            drawRectFill(offset + Point2I(x, 0), Point2I(d, d), mFgColor);
            drawRectFill(offset + Point2I(x, H - d), Point2I(d, d), mFgColor);
        }
        for (S32 y = 0; y < H; y += d + gap)
        {
            drawRectFill(offset + Point2I(0, y), Point2I(d, d), mFgColor);
            drawRectFill(offset + Point2I(W - d, y), Point2I(d, d), mFgColor);
        }
    }

    // Child controls
    renderChildControls(offset, updateRect);

    // Crosshair & Highlight
    if (mShowCrosshair && (mHovering || mDragging))
    {
        Point2I cp = mCrossPos;
        cp.x = getMax(0, getMin(W, cp.x));
        cp.y = getMax(0, getMin(H, cp.y));

        for (S32 t = 0; t < mLineThickness; ++t)
        {
            drawLine(offset + Point2I(cp.x + t, 0), offset + Point2I(cp.x + t, H), mFgColor);
            drawLine(offset + Point2I(0, cp.y + t), offset + Point2I(W, cp.y + t), mFgColor);
        }

        const S32 sq = 8, half = sq / 2;
        Point2I tl = offset + cp - Point2I(half, half);
        drawRectFill(tl, Point2I(sq, mLineThickness), mFgColor);
        drawRectFill(tl + Point2I(0, sq - mLineThickness), Point2I(sq, mLineThickness), mFgColor);
        drawRectFill(tl, Point2I(mLineThickness, sq), mFgColor);
        drawRectFill(tl + Point2I(sq - mLineThickness, 0), Point2I(mLineThickness, sq), mFgColor);

        S32 cellX = (cp.x / mGridSpacing) * mGridSpacing;
        S32 cellY = (cp.y / mGridSpacing) * mGridSpacing;
        Point2I cOff(offset.x + cellX, offset.y + cellY);
        drawRectFill(cOff, Point2I(mGridSpacing, mLineThickness), mFgColor);
        drawRectFill(cOff + Point2I(0, mGridSpacing - mLineThickness), Point2I(mGridSpacing, mLineThickness), mFgColor);
        drawRectFill(cOff, Point2I(mLineThickness, mGridSpacing), mFgColor);
        drawRectFill(cOff + Point2I(mGridSpacing - mLineThickness, 0), Point2I(mLineThickness, mGridSpacing), mFgColor);
    }
}

void GuiGridContainerCtrl::onControlDragged(GuiControl* control, const Point2I& dropPoint) {
    control->setPosition(snapToGrid(globalToLocalCoord(dropPoint)));
}

void GuiGridContainerCtrl::onControlDropped(GuiControl* control, const Point2I& dropPoint) {
    control->setPosition(snapToGrid(globalToLocalCoord(dropPoint)));
}
