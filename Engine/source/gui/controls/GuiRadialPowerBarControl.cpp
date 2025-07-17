//------------------------------------------------------------------------------
// GuiRadialPowerBarControl.cpp
//------------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"            // for IMPLEMENT_CONOBJECT
#include "core/util/tVector.h"            // for Vector<>
#include "guiRadialPowerBarControl.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/primBuilder.h"              // for Triangles
#include "math/mMathFn.h"

IMPLEMENT_CONOBJECT(GuiRadialPowerBarControl);

GuiRadialPowerBarControl::GuiRadialPowerBarControl()
   : mPower(0),
   mMaxPower(5000),
   mIncrement(1000),
   mStartAngle(-90.f),
   mEndAngle(90.f),
   mRadius(50.f),
   mCircleRadius(6),
   mCenterOffset(Point2F(60, 60)),
   mFilledColor(0, 150, 255, 255),
   mEmptyColor(80, 80, 80, 200)
{
}

void GuiRadialPowerBarControl::initPersistFields()
{
   Parent::initPersistFields();
   addField("power", TypeS32, Offset(mPower, GuiRadialPowerBarControl), "Current power");
   addField("maxPower", TypeS32, Offset(mMaxPower, GuiRadialPowerBarControl), "Maximum power");
   addField("increment", TypeS32, Offset(mIncrement, GuiRadialPowerBarControl), "Power step per circle");
   addField("startAngle", TypeF32, Offset(mStartAngle, GuiRadialPowerBarControl), "Arc start angle (deg)");
   addField("endAngle", TypeF32, Offset(mEndAngle, GuiRadialPowerBarControl), "Arc end angle (deg)");
   addField("radius", TypeF32, Offset(mRadius, GuiRadialPowerBarControl), "Arc radius");
   addField("circleRadius", TypeS32, Offset(mCircleRadius, GuiRadialPowerBarControl), "Circle radius");
   addField("centerOffset", TypePoint2F, Offset(mCenterOffset, GuiRadialPowerBarControl), "Center XY offset");
   addField("filledColor", TypeColorI, Offset(mFilledColor, GuiRadialPowerBarControl), "Color for filled circles");
   addField("emptyColor", TypeColorI, Offset(mEmptyColor, GuiRadialPowerBarControl), "Color for empty circles");
}

void GuiRadialPowerBarControl::onRender(Point2I offset, const RectI& updateRect)
{
   Parent::onRender(offset, updateRect);

   // Total segments (circles)
   const S32 segments = mMaxPower / mIncrement;
   if (segments < 1)
      return;

   // Precompute all circle center positions
   Vector<Point2F> centers;
   centers.setSize(segments);

   for (S32 i = 0; i < segments; ++i)
   {
      F32 t = (segments > 1) ? F32(i) / (segments - 1) : 0.5f;
      F32 angleDeg = mLerp(mStartAngle, mEndAngle, t);
      F32 angleRad = mDegToRad(angleDeg);

      F32 cx = offset.x + mCenterOffset.x + mCos(angleRad) * mRadius;
      F32 cy = offset.y + mCenterOffset.y + mSin(angleRad) * mRadius;

      centers[i] = Point2F(cx, cy);
   }

   const F32 halfThick = mCircleRadius * 0.5f;

   // Draw connectors between filled circles
   for (S32 i = 0; i + 1 < segments; ++i)
   {
      bool isFilled = (mPower >= (i + 2) * mIncrement);
      ColorI color = isFilled ? mFilledColor : mEmptyColor;

      const Point2F& a = centers[i];
      const Point2F& b = centers[i + 1];

      Point2F dir = b - a;
      dir.normalize();
      Point2F perp(-dir.y, dir.x);

      Point2F v0 = a + perp * halfThick;
      Point2F v1 = a - perp * halfThick;
      Point2F v2 = b - perp * halfThick;
      Point2F v3 = b + perp * halfThick;

      PrimBuild::begin(GFXTriangleList, 6);
      PrimBuild::color(color);
      PrimBuild::vertex2fv(v0);
      PrimBuild::vertex2fv(v1);
      PrimBuild::vertex2fv(v2);
      PrimBuild::vertex2fv(v0);
      PrimBuild::vertex2fv(v2);
      PrimBuild::vertex2fv(v3);
      PrimBuild::end();
   }

   // Draw circles on top of connectors
   const U32 detail = 16;
   const F32 twoPI = M_PI_F * 2.0f;

   for (S32 i = 0; i < segments; ++i)
   {
      bool filled = (mPower >= (i + 1) * mIncrement);
      ColorI col = filled ? mFilledColor : mEmptyColor;

      const F32 cx = centers[i].x;
      const F32 cy = centers[i].y;

      PrimBuild::begin(GFXTriangleList, detail * 3);
      PrimBuild::color(col);
      Point3F centerPt(cx, cy, 0.0f);

      for (U32 j = 0; j < detail; ++j)
      {
         F32 theta0 = twoPI * (F32(j) / detail);
         F32 theta1 = twoPI * (F32(j + 1) / detail);

         Point3F p0(cx + mCos(theta0) * mCircleRadius,
            cy + mSin(theta0) * mCircleRadius, 0.0f);

         Point3F p1(cx + mCos(theta1) * mCircleRadius,
            cy + mSin(theta1) * mCircleRadius, 0.0f);

         PrimBuild::vertex3fv(centerPt);
         PrimBuild::vertex3fv(p0);
         PrimBuild::vertex3fv(p1);
      }

      PrimBuild::end();
   }
}

bool GuiRadialPowerBarControl::onAdd()
{
   if (!Parent::onAdd())
      return false;

   setProcessTicks(true);
   return true;
}

void GuiRadialPowerBarControl::onRemove()
{
   setProcessTicks(false);
   Parent::onRemove();
}

void GuiRadialPowerBarControl::processTick()
{
   // Optional: add health regen cost logic here.
   // Or just use advanceTime() if it's time-based.
}

DefineEngineMethod(GuiRadialPowerBarControl, setUpdate, void, (), ,
   "Marks the radial power control for redraw.")
{
   object->setUpdate();
}

DefineEngineMethod(GuiRadialPowerBarControl, getHealth, F32, (), ,
   "Returns the current power (health) value.")
{
   return object->mPower;
}

DefineEngineMethod(GuiRadialPowerBarControl, setHealth, void, (F32 value), ,
   "Sets the current power (health) value.")
{
   object->mPower = mClampF(value, 0.f, object->mMaxPower);
   object->setUpdate();
}

DefineEngineMethod(GuiRadialPowerBarControl, getMaxHealth, F32, (), ,
   "Returns the maximum power (health) value.")
{
   return object->mMaxPower;
}

DefineEngineMethod(GuiRadialPowerBarControl, setMaxHealth, void, (F32 value), ,
   "Sets the maximum power (health) value.")
{
   object->mMaxPower = value;
   object->setUpdate();
}
