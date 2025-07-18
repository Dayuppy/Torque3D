
#include "guiCompassControl.h"
#include "gfx/gfxDrawUtil.h"
#include "math/mMathFn.h"
#include "math/mathUtils.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/gameBase.h"

IMPLEMENT_CONOBJECT(GuiCompassControl);

GuiCompassControl::GuiCompassControl()
{
   mScaleX = 1.0f;
   mScaleY = 1.0f;
   mDrawFrame = true;
   mDrawShadows = true;
   mHashmarkSpacing = 5;
   mFontSize = 14;
   mPrevYaw = 0.f;

   mFrameColor.set(0, 0, 0, 128);
   mHashmarkColor.set(255, 255, 255, 255);
   mNeedleColor.set(255, 0, 0, 255);
   mDegreeColor.set(255, 0, 0, 255);
   mNorthColor.set(255, 255, 0, 255);
}

void GuiCompassControl::initPersistFields()
{
   Parent::initPersistFields();
   addField("scaleX", TypeF32, Offset(mScaleX, GuiCompassControl));
   addField("scaleY", TypeF32, Offset(mScaleY, GuiCompassControl));
   addField("drawFrame", TypeBool, Offset(mDrawFrame, GuiCompassControl));
   addField("drawShadows", TypeBool, Offset(mDrawShadows, GuiCompassControl));
   addField("hashmarkSpacing", TypeS32, Offset(mHashmarkSpacing, GuiCompassControl));
   addField("fontSize", TypeS32, Offset(mFontSize, GuiCompassControl));
   addField("frameColor", TypeColorI, Offset(mFrameColor, GuiCompassControl));
   addField("hashmarkColor", TypeColorI, Offset(mHashmarkColor, GuiCompassControl));
   addField("needleColor", TypeColorI, Offset(mNeedleColor, GuiCompassControl));
   addField("degreeColor", TypeColorI, Offset(mDegreeColor, GuiCompassControl));
   addField("northColor", TypeColorI, Offset(mNorthColor, GuiCompassControl));
}

void GuiCompassControl::onRender(Point2I offset, const RectI& updateRect)
{
   GameConnection* conn = GameConnection::getConnectionToServer();
   if (!conn) return;
   GameBase* control = dynamic_cast<GameBase*>(conn->getControlObject());
   if (!control) return;

   VectorF fwd = control->getTransform().getForwardVector();
   F32 yaw = mRadToDeg(mAtan2(fwd.x, fwd.y));
   yaw = mFmod(yaw + 360.0f, 360.0f);

   F32 diff = mFmod(yaw - mPrevYaw + 540.0f, 360.0f) - 180.0f;
   F32 smoothedYaw = mFmod(mPrevYaw + diff * 0.2f + 360.0f, 360.0f);
   mPrevYaw = smoothedYaw;

   GFXDrawUtil* drawer = GFX->getDrawUtil();
   GFont* font = mProfile->mFont;

   if (!font)
      return; // gracefully skip rendering

   Point2I extent = getExtent();
   F32 width = extent.x * mScaleX;
   F32 height = 60 * mScaleY;
   F32 x = (extent.x - width) / 2;
   F32 y = 10;
   S32 centerX = (S32)(x + (width / 2));

   if (mDrawFrame)
      drawer->drawRectFill(RectI((S32)(x - 10), (S32)(y - 10), (S32)(width + 20), (S32)(height + 30)), mFrameColor);

   for (S32 a = 0; a < 360; a += mHashmarkSpacing)
   {
      S32 rel = a - (S32)smoothedYaw;
      if (rel < -180) rel += 360;
      if (rel > 180)  rel -= 360;
      if (rel < -60 || rel > 60) continue;

      S32 xMark = (S32)(((rel + 60) / 120.0f) * width + x);
      S32 h = (a % 15 == 0) ? (S32)(15 * mScaleY) : (S32)(7 * mScaleY);
      drawer->drawLine(Point2I(xMark, (S32)(y + height - h - 8)), Point2I(xMark, (S32)(y + height - 8)), mHashmarkColor);

      if (a % 15 == 0)
      {
         char buf[8]; dSprintf(buf, sizeof(buf), "%d", a);
         drawer->drawTextShadowed(font, Point2I(xMark - 6, (S32)(y + height)), buf, mHashmarkColor, ColorI(0, 0, 0, 255));
      }
   }

   drawer->drawLine(Point2I(centerX, (S32)(y + height - 35)), Point2I(centerX, (S32)(y + height - 5)), mNeedleColor);

   char degBuf[8]; dSprintf(degBuf, sizeof(degBuf), "%d°", (S32)smoothedYaw);
   drawer->drawTextShadowed(font, Point2I(centerX - 12, (S32)(y + height + 12)), degBuf, mDegreeColor, ColorI(0, 0, 0, 255));

   for (S32 a = 0; a < 360; a += 15)
   {
      S32 rel = a - (S32)smoothedYaw;
      if (rel < -180) rel += 360;
      if (rel > 180)  rel -= 360;
      if (rel < -60 || rel > 60) continue;

      const char* label = "";
      switch (a)
      {
      case 0: label = "N"; break; case 45: label = "NE"; break; case 90: label = "E"; break;
      case 135: label = "SE"; break; case 180: label = "S"; break; case 225: label = "SW"; break;
      case 270: label = "W"; break; case 315: label = "NW"; break;
      }

      if (label[0])
      {
         S32 xText = (S32)(((rel + 60) / 120.0f) * width + x) - 6;
         ColorI col = (a == 0) ? mNorthColor : mHashmarkColor;
         drawer->drawTextShadowed(font, Point2I(xText, (S32)y), label, col, ColorI(0, 0, 0, 255));
      }
   }
}
