
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
   // Get the player’s yaw
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
   if (!font) return;

   // Compute layout
   Point2I extent = getExtent();
   F32 width = extent.x * mScaleX;
   F32 height = 60 * mScaleY;
   F32 x = (extent.x - width) / 2;
   F32 y = 10;
   S32 centerX = (S32)(x + width * 0.5f);

   // --- ROUNDED FRAME ---
   if (mDrawFrame)
   {
      RectI frameRect(
         (S32)(x - 5),
         (S32)(y - 5),
         (S32)(width + 10),
         (S32)(height + 15)
      );

      // drawRoundedRect(cornerRadius, rect, fillColor, borderSize, borderColor)
      drawer->drawRoundedRect(
         mFrameCornerRadius,
         frameRect,
         mFrameColor,
         0.25f,            // no border
         mFrameColor      // border color (ignored when borderSize=0)
      );
   }

   // Hashmarks + numbers
   for (S32 a = 0; a < 360; a += mHashmarkSpacing)
   {
      S32 rel = a - (S32)smoothedYaw;
      if (rel < -180) rel += 360;
      if (rel > 180) rel -= 360;
      if (rel < -60 || rel > 60) continue;

      S32 xMark = (S32)(((rel + 60) / 120.0f) * width + x);
      S32 h = (a % 15 == 0) ? (S32)(15 * mScaleY) : (S32)(7 * mScaleY);
      drawer->drawLine(
         Point2I(xMark, (S32)(y + height - h - 8)),
         Point2I(xMark, (S32)(y + height - 8)),
         mHashmarkColor);

      // Major tick labels
      if (a % 15 == 0)
      {
         char buf[8];
         dSprintf(buf, sizeof(buf), "%d", a);
         Point2I textPos(xMark - 6, (S32)(y + height));
         if (mDrawShadows)
            drawer->drawTextShadowed(
               font, textPos, Point2I(2, 2),
               buf, mHashmarkColor, ColorI(0, 0, 0, 255));
         else
            drawer->drawText(font, textPos, buf, &mHashmarkColor);
      }
   }

   // Center needle
   drawer->drawLine(
      Point2I(centerX, (S32)(y + height - 35)),
      Point2I(centerX, (S32)(y + height - 5)),
      mNeedleColor);

   // Degree text
   {
      char degBuf[8];
      dSprintf(degBuf, sizeof(degBuf), "%d°", (S32)smoothedYaw);
      Point2I degPos(centerX - 12, (S32)(y + height + 12));
      if (mDrawShadows)
         drawer->drawTextShadowed(
            font, degPos, Point2I(2, 2),
            degBuf, mDegreeColor, ColorI(0, 0, 0, 255));
      else
         drawer->drawText(font, degPos, degBuf, &mDegreeColor);
   }

   // Cardinal labels
   static const char* labels[8] = { "N","NE","E","SE","S","SW","W","NW" };
   for (S32 i = 0; i < 8; ++i)
   {
      S32 a = i * 45;
      S32 rel = a - (S32)smoothedYaw;
      if (rel < -180) rel += 360;
      if (rel > 180) rel -= 360;
      if (rel < -60 || rel > 60) continue;

      S32 xText = (S32)(((rel + 60) / 120.0f) * width + x) - 6;
      Point2I labelPos(xText, (S32)y);
      ColorI col = (a == 0) ? mNorthColor : mHashmarkColor;

      if (mDrawShadows)
         drawer->drawTextShadowed(
            font, labelPos, Point2I(2, 2),
            labels[i], col, ColorI(0, 0, 0, 255));
      else
         drawer->drawText(font, labelPos, labels[i], &col);
   }

   // Render any child controls (selection, etc.)
   renderChildControls(offset, updateRect);
}

