#include "GuiProgressBarControl.h"
#include "console/engineAPI.h"
#include "platform/platform.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT(GuiProgressBarControl);

GuiProgressBarControl::GuiProgressBarControl()
   : mValue(100.f),
   mMaxValue(100.f),
   mSmoothValue(100.f),
   mBarColor(0, 200, 0, 255),
   mBackColor(50, 50, 50, 200),
   mDrawFrame(true),
   mFrameColor(255, 255, 255, 255),
   mDrawPercentage(false)
{
}

void GuiProgressBarControl::initPersistFields()
{
   Parent::initPersistFields();

   addField("value", TypeF32, Offset(mValue, GuiProgressBarControl));
   addField("maxValue", TypeF32, Offset(mMaxValue, GuiProgressBarControl));
   addField("barColor", TypeColorI, Offset(mBarColor, GuiProgressBarControl));
   addField("backColor", TypeColorI, Offset(mBackColor, GuiProgressBarControl));
   addField("drawFrame", TypeBool, Offset(mDrawFrame, GuiProgressBarControl));
   addField("frameColor", TypeColorI, Offset(mFrameColor, GuiProgressBarControl));
   addField("drawPercentage", TypeBool, Offset(mDrawPercentage, GuiProgressBarControl));
}

bool GuiProgressBarControl::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void GuiProgressBarControl::onRemove()
{
   Parent::onRemove();
}

void GuiProgressBarControl::setValue(F32 v)
{
   F32 clamped = mClampF(v, 0.f, mMaxValue);
   if (mFabs(clamped - mValue) > 0.01f)
   {
      mValue = clamped;
      setUpdate();
   }
}

void GuiProgressBarControl::setMaxValue(F32 v)
{
   F32 clamped = mClampF(v, 0.f, FLT_MAX);
   if (mFabs(clamped - mMaxValue) > 0.01f)
   {
      mMaxValue = clamped;
      mValue = mClampF(mValue, 0.f, mMaxValue);
      setUpdate();
   }
}

void GuiProgressBarControl::onRender(Point2I offset, const RectI& updateRect)
{
   GFXDrawUtil* drawer = GFX->getDrawUtil();
   Point2I ext = getExtent();
   S32 w = ext.x, h = ext.y;

   F32 ratio = (mMaxValue > 0.f) ? mClampF(mValue / mMaxValue, 0.f, 1.f) : 0.f;
   mSmoothValue = mLerp(mSmoothValue, ratio, 0.15f);
   S32 fillW = S32(mSmoothValue * w);

   ColorI fill = mBarColor;
   if (ratio < 0.25f)
   {
      F32 t = Platform::getVirtualMilliseconds() * 0.005f;
      F32 pulse = 0.5f + 0.5f * mSin(t);
      fill.red = U8(mLerp((F32)mBackColor.red, (F32)mBarColor.red, pulse));
      fill.green = U8(mLerp((F32)mBackColor.green, (F32)mBarColor.green, pulse));
      fill.blue = U8(mLerp((F32)mBackColor.blue, (F32)mBarColor.blue, pulse));
      fill.alpha = U8(mLerp((F32)mBackColor.alpha, (F32)mBarColor.alpha, pulse));
   }

   drawer->drawRectFill(RectI(offset, ext), mBackColor);

   if (fillW > 0)
   {
      RectI fillRect(offset.x, offset.y, fillW, h);
      drawer->drawRectFill(fillRect, fill);
   }

   if (mDrawFrame)
      drawer->drawRect(RectI(offset, ext), mFrameColor);

   if (mDrawPercentage)
   {
      String percentText = String::ToString("%d%%", (S32)(ratio * 100.f));
      GFont* font = mProfile->mFont;
      if (font)
      {
         S32 textWidth = font->getStrWidth(percentText.c_str());
         S32 textHeight = font->getHeight();
         Point2I center(offset.x + (w - textWidth) / 2, offset.y + (h - textHeight) / 2);
         drawer->drawText(font, center, percentText, &mProfile->mFontColor);
      }
   }

   renderChildControls(offset, updateRect);
}

// Console methods
DefineEngineMethod(GuiProgressBarControl, getValue, F32, (), , "Returns current bar value.") { return object->getValue(); }
DefineEngineMethod(GuiProgressBarControl, setValue, void, (F32 v), , "Sets current bar value.") { object->setValue(v); }
DefineEngineMethod(GuiProgressBarControl, getMaxValue, F32, (), , "Returns max bar value.") { return object->getMaxValue(); }
DefineEngineMethod(GuiProgressBarControl, setMaxValue, void, (F32 v), , "Sets max bar value.") { object->setMaxValue(v); }
DefineEngineMethod(GuiProgressBarControl, setUpdate, void, (), , "Marks control for redraw.") { object->setUpdate(); }
DefineEngineMethod(GuiProgressBarControl, setDrawPercentage, void, (bool on), , "Toggles percentage text.") { object->mDrawPercentage = on; object->setUpdate(); }
