
#ifndef _GUI_COMPASS_CONTROL_H_
#define _GUI_COMPASS_CONTROL_H_

#include "gui/core/guiControl.h"
#include "gfx/gfxDevice.h"
#include "math/mPoint2.h"

class GuiCompassControl : public GuiControl
{
   typedef GuiControl Parent;

public:
   DECLARE_CONOBJECT(GuiCompassControl);

   GuiCompassControl();

   static void initPersistFields();
   virtual void onRender(Point2I offset, const RectI& updateRect) override;

   // Layout and rendering options
   F32 mScaleX;
   F32 mScaleY;
   bool mDrawFrame;
   bool mDrawShadows;
   S32 mHashmarkSpacing;
   S32 mFontSize;

   // Bar Compass
   ColorI mFrameColor;
   ColorI mHashmarkColor;
   ColorI mNeedleColor;
   ColorI mDegreeColor;
   ColorI mNorthColor;

   // Radial Compass
   Point2F mRadialCenterOffset;
   F32     mRadialRadius;
   F32     mRadialTickLength;
   F32     mRadialTextDistance;

   // State
   F32 mPrevYaw;
};

#endif // _GUI_COMPASS_CONTROL_H_
