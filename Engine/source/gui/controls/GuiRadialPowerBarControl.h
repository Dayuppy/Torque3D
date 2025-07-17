#ifndef _GUIRADIALPOWERBARCONTROL_H_
#define _GUIRADIALPOWERBARCONTROL_H_

#include "gui/core/guiControl.h"
#include "console/consoleTypes.h"
#include "core/iTickable.h"

class GuiRadialPowerBarControl : public GuiControl, public virtual ITickable
{
   typedef GuiControl Parent;

protected:

public:
   DECLARE_CONOBJECT(GuiRadialPowerBarControl);
   GuiRadialPowerBarControl();

   static void initPersistFields();
   virtual void onRender(Point2I offset, const RectI& updateRect) override;

   virtual bool onAdd() override;
   virtual void onRemove() override;

   virtual void processTick() override;
   virtual void advanceTime(F32) override {}
   virtual void interpolateTick(F32) override {}

   S32 mPower, mMaxPower, mIncrement;
   F32 mStartAngle, mEndAngle, mRadius;
   S32 mCircleRadius;
   Point2F mCenterOffset;
   ColorI mFilledColor, mEmptyColor;
};

#endif // _GUIRADIALPOWERBARCONTROL_H_
