#ifndef _GUI_RADIAL_COMPASS_CONTROL_H_
#define _GUI_RADIAL_COMPASS_CONTROL_H_

#include "gui/core/guiControl.h"
#include "math/mMathFn.h"
#include "gfx/gfxDrawUtil.h"

class GuiRadialCompassControl : public GuiControl
{
	typedef GuiControl Parent;

protected:
	Point2F mCenterOffset;
	F32       mRadius;
	F32       mTickLength;
	F32       mTextDistance;
	ColorI    mHashmarkColor;
	ColorI    mNorthColor;
	ColorI    mNeedleColor;
	ColorI    mDegreeColor;
	F32       mPrevYaw;

public:
	DECLARE_CONOBJECT(GuiRadialCompassControl);

	GuiRadialCompassControl();
	static void initPersistFields();
	virtual void onRender(Point2I offset, const RectI& updateRect) override;

	Point2F mRadialCenterOffset;
};

#endif // _GUI_RADIAL_COMPASS_CONTROL_H_
