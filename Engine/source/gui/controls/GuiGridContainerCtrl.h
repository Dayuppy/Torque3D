// GuiGridContainerCtrl.h
#ifndef _GUIGRIDCONTAINERCTRL_H_
#define _GUIGRIDCONTAINERCTRL_H_

#include "gui/core/guiControl.h"
#include "console/consoleTypes.h"

class GuiGridContainerCtrl : public GuiControl {
	typedef GuiControl Parent;

public:
	DECLARE_CONOBJECT(GuiGridContainerCtrl);

	GuiGridContainerCtrl();
	virtual ~GuiGridContainerCtrl() {}

	// Render the grid background
	virtual void onRender(Point2I offset, const RectI& updateRect) override;

	// --- drag overrides ---
	virtual void onMouseDragged(const GuiEvent& event) override;
	virtual void onMouseMove(const GuiEvent& event) override;  ///< NEW
	virtual void onMouseUp(const GuiEvent& event) override;
	virtual void onMouseEnter(const GuiEvent& event) override;  ///< NEW
	virtual void onMouseLeave(const GuiEvent& event) override;  ///< NEW

	// Snap children to grid when dragged or dropped
	virtual void onControlDragged(GuiControl* control, const Point2I& dropPoint);
	virtual void onControlDropped(GuiControl* control, const Point2I& dropPoint);

	// Persist fields
	static void initPersistFields();

protected:
	S32 mGridSpacing;
	ColorI mBgColor;
	ColorI mFgColor;
	S32 mLineThickness;

	// drag/crosshair state
	bool    mDragging;
	bool    mHovering;         ///< now track hover as well
	Point2I mCrossPos;         ///< the snapped-to position for both drag & hover

	bool    mShowCrosshair;
	bool  mDottedGrid;     ///< when true, grid lines are dotted instead of solid

	Point2I snapToGrid(const Point2I& pt) const;
};

#endif // _GUIGRIDCONTAINERCTRL_H_