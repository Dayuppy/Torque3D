#include "platform/platform.h"
#include "gfx/gfxDrawUtil.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "guiTextEditColorCtrl.h"
#include "gui/core/guiDefaultControlRender.h"

IMPLEMENT_CONOBJECT(GuiTextEditColorCtrl);

GuiTextEditColorCtrl::GuiTextEditColorCtrl()
{
   mUseShadow = false;
   mShadowColor.set(0, 0, 0, 255);
   mShadowOffset.set(2, 2);

   mUseOutline = false;
   mOutlineColor.set(0, 0, 0, 255);
   mOutlineThickness = 1;

   mAlignment = GuiControlProfile::LeftJustify;
   mFontName = StringTable->insert("Arial");
   mFontSize = 20;
   mFontColor.set(255, 255, 255, 255);

   mWordWrap = false;

   mDrawFrame = true;
   mFrameColor.set(0, 0, 0, 127); // black, semi-transparent
   mPlaceholderText = StringTable->insert("Placeholder Text");
   mPlaceholderColor.set(200, 200, 200, 255); // light gray
   mContainerOpaque = true;
   mProfile = dynamic_cast<GuiControlProfile*>(Sim::findObject("GuiTextEditProfile"));
}

void GuiTextEditColorCtrl::initPersistFields()
{
   addField("useShadow", TypeBool, Offset(mUseShadow, GuiTextEditColorCtrl), "Render a drop shadow");
   addField("shadowColor", TypeColorI, Offset(mShadowColor, GuiTextEditColorCtrl), "Shadow color");
   addField("shadowOffset", TypePoint2I, Offset(mShadowOffset, GuiTextEditColorCtrl), "Shadow offset");
   addField("useOutline", TypeBool, Offset(mUseOutline, GuiTextEditColorCtrl), "Render an outline");
   addField("outlineColor", TypeColorI, Offset(mOutlineColor, GuiTextEditColorCtrl), "Outline color");
   addField("outlineThickness", TypeS32, Offset(mOutlineThickness, GuiTextEditColorCtrl), "Outline thickness");
   addField("justify", TypeGuiAlignmentType, Offset(mAlignment, GuiTextEditColorCtrl), "Text justification");
   addField("wordWrap", TypeBool, Offset(mWordWrap, GuiTextEditColorCtrl), "Enable word wrapping");
   addField("fontName", TypeCaseString, Offset(mFontName, GuiTextEditColorCtrl), "Font face name");
   addField("fontSize", TypeS32, Offset(mFontSize, GuiTextEditColorCtrl), "Font size");
   addField("fontColor", TypeColorI, Offset(mFontColor, GuiTextEditColorCtrl), "Font color");
   addField("drawFrame", TypeBool, Offset(mDrawFrame, GuiTextEditColorCtrl), "Draw background frame.");
   addField("frameColor", TypeColorI, Offset(mFrameColor, GuiTextEditColorCtrl), "Color of background frame.");
   addField("placeholderColor", TypeColorI, Offset(mPlaceholderColor, GuiTextEditColorCtrl), "Color for placeholder text.");
   addField("containerOpaque", TypeBool, Offset(mContainerOpaque, GuiTextEditColorCtrl), "If true, this control will capture all mouse input.");

   Parent::initPersistFields();
}

bool GuiTextEditColorCtrl::onWake()
{
   if (!Parent::onWake())
      return false;

   refreshFont();

   return true;
}

void GuiTextEditColorCtrl::refreshFont()
{
   if (mFontName && mFontName[0] && mFontSize > 0)
   {
      Resource<GFont> newFont = GFont::create(mFontName, mFontSize);
      if (newFont)
      {
         mCustomFont = newFont;

         // Adjust height
         Point2I extent = getExtent();
         extent.y = mCustomFont->getHeight() + 6;
         setExtent(extent);
      }
      else
         Con::warnf("GuiTextEditColorCtrl::refreshFont - Failed to load font %s size %d", mFontName, mFontSize);
   }
   else
   {
      mCustomFont = nullptr;
   }
}

void GuiTextEditColorCtrl::onMouseDown(const GuiEvent& event)
{
   if (!mActive)
      return;

   setFirstResponder();
   mouseLock();

   Parent::onMouseDown(event);
}

GuiControl* GuiTextEditColorCtrl::findHitControl(const Point2I& pt, S32 initialLayer)
{
   RectI myRect(Point2I::Zero, getExtent());
   if (mContainerOpaque && myRect.pointInRect(pt))
      return this;

   return Parent::findHitControl(pt, initialLayer);
}

//-----------------------------------------------------------------------------
// GuiTextEditColorCtrl::drawText   – final fixed version
//-----------------------------------------------------------------------------
void GuiTextEditColorCtrl::drawText(const RectI& drawRect, bool isFocused)
{
   //-----------------------------------------------------------------
   // 0)  Font in use
   //-----------------------------------------------------------------
   GFont* font = mCustomFont ? mCustomFont : mProfile->mFont;
   if (!font)
      return;

   //-----------------------------------------------------------------
   // 1)  Build string (password masking & placeholder support)
   //-----------------------------------------------------------------
   StringBuffer textBuf;
   char raw[GuiTextCtrl::MAX_STRING_LENGTH + 1] = { 0 };
   getRenderText(raw);

   if (mPasswordText)
   {
      for (U32 i = 0, n = dStrlen(raw); i < n; ++i)
         textBuf.append(mPasswordMask);
   }
   else
      textBuf.set(raw);

   const bool showPlaceholder =
      (textBuf.length() == 0) && !isFocused && *mPlaceholderText;
   if (showPlaceholder)
      textBuf.set(mPlaceholderText);

   if (mCursorPos > textBuf.length())
      mCursorPos = textBuf.length();

   //-----------------------------------------------------------------
   // 2)  Initial draw position (before horizontal scroll adjustment)
   //-----------------------------------------------------------------
   Point2I padLT(mProfile->mTextOffset.x ? mProfile->mTextOffset.x : 3,
      mProfile->mTextOffset.y);
   Point2I padRB = padLT;

   Point2I base = drawRect.point;
   base.y += (drawRect.extent.y - padLT.y - padRB.y
      - font->getHeight()) / 2 + padLT.y;

   const S32 strPx = font->getStrNWidth(textBuf.getPtr(),
      textBuf.length());

   switch (mProfile->mAlignment)
   {
   case GuiControlProfile::RightJustify:
      base.x += drawRect.extent.x - strPx - padRB.x;  break;
   case GuiControlProfile::CenterJustify:
      base.x += (drawRect.extent.x - strPx) / 2;     break;
   default:                                           // LeftJustify
      base.x += padLT.x;                               break;
   }

   mTextOffset = base;   // save global origin for caret/scrolling

   //-----------------------------------------------------------------
   // 3)  Caret-scroll maths (unchanged from previous version)
   //-----------------------------------------------------------------
   if (isFocused && mActive)
   {
      const S32 caretPx =
         mCursorPos ? font->getStrNWidth(textBuf.getPtr(), mCursorPos) : 0;

      if (mTextOffset.x + caretPx + 1 >= drawRect.point.x + drawRect.extent.x)
         mTextOffset.x =
         drawRect.point.x + drawRect.extent.x - 1 - caretPx;
      else if (mTextOffset.x + caretPx < drawRect.point.x + padLT.x)
         mTextOffset.x =
         drawRect.point.x + padLT.x - caretPx;

      if (mCursorOn)
      {
         const Point2I cStart(mTextOffset.x + caretPx, base.y);
         const Point2I cEnd(cStart.x,
            cStart.y + font->getHeight());
         GFX->getDrawUtil()->drawLine(cStart, cEnd, mProfile->mCursorColor);
      }
   }

   //-----------------------------------------------------------------
   // 4)  Selection highlight
   //-----------------------------------------------------------------
   if (mBlockEnd > mBlockStart)
   {
      const UTF16* u16 = textBuf.getPtr();
      const S32 selStartPx = font->getStrNWidth(u16, mBlockStart);
      const S32 selPx =
         font->getStrNWidth(u16 + mBlockStart, mBlockEnd - mBlockStart);

      Point2I ul(mTextOffset.x + selStartPx, drawRect.point.y);
      Point2I lr(ul.x + selPx, ul.y + drawRect.extent.y - 1);
      GFX->getDrawUtil()->drawRectFill(ul, lr, mProfile->mFontColorSEL);
   }

   //-----------------------------------------------------------------
   // 5)  Draw the glyphs in the requested style – **colour always honoured**
   //-----------------------------------------------------------------
   const ColorI glyphColour = showPlaceholder ? mPlaceholderColor : mFontColor;
   const char* finalStr = textBuf.getPtr8();
   GFXDrawUtil* du = GFX->getDrawUtil();

   if (mUseShadow && mUseOutline)
   {
      du->drawTextShadowed(font, mTextOffset, mShadowOffset,
         finalStr, glyphColour, mShadowColor);
      du->drawTextOutlined(font, mTextOffset,
         finalStr, glyphColour,
         mOutlineColor, mOutlineThickness);
   }
   else if (mUseShadow)
   {
      du->drawTextShadowed(font, mTextOffset, mShadowOffset,
         finalStr, glyphColour, mShadowColor);
   }
   else if (mUseOutline)
   {
      du->drawTextOutlined(font, mTextOffset,
         finalStr, glyphColour,
         mOutlineColor, mOutlineThickness);
   }
   else
   {
      // Explicitly set modulation colour so the correct colour is used even
      // when neither shadow nor outline is enabled.
      du->setBitmapModulation(glyphColour);
      du->drawText(font, mTextOffset, finalStr);
      du->clearBitmapModulation();   // restore state
   }
}

void GuiTextEditColorCtrl::onRender(Point2I offset, const RectI& updateRect)
{
   GFXDrawUtil* du = GFX->getDrawUtil();
   RectI ctrlRect(offset, getExtent());

   //-----------------------------------------------------------------------
   // 1)  Frame / background
   //-----------------------------------------------------------------------
   if (mDrawFrame)
   {
      du->drawRectFill(ctrlRect, mFrameColor);         // fill
      du->drawRect(ctrlRect, mProfile->mBorderColor); // outline
   }

   //-----------------------------------------------------------------------
   // 2)  Choose a font (needed for auto-grow below)
   //-----------------------------------------------------------------------
   GFont* font = mCustomFont ? mCustomFont : mProfile->mFont;
   if (!font)
   {
      // No font?  At least let the parent draw caret / selection safely.
      Parent::onRender(offset, updateRect);
      return;
   }

   //-----------------------------------------------------------------------
   // 3)  Compute string width *once* to auto-grow horizontally when needed.
   //     We don’t draw here – glyphs are rendered inside drawText().
   //-----------------------------------------------------------------------
   char txt[GuiTextCtrl::MAX_STRING_LENGTH + 1] = { 0 };
   getText(txt);

   if (txt[0] == '\0' && !isFirstResponder())
      dStrncpy(txt, mPlaceholderText, GuiTextCtrl::MAX_STRING_LENGTH);

   const S32 neededWidth = font->getStrWidth(txt) + 8;       // +padding
   if (neededWidth > getExtent().x)
   {
      Point2I ext = getExtent();
      ext.x = neededWidth;
      setExtent(ext);
      // NB: setExtent() triggers onRender during the same frame, but because
      // we’re still inside onRender() that’s harmless.
   }

   //-----------------------------------------------------------------------
   // 4)  Let the stock engine do caret / selection & (via our drawText)
   //     the actual glyph rendering.  This guarantees glyphs are drawn
   //     exactly once with your custom style.
   //-----------------------------------------------------------------------
   Parent::onRender(offset, updateRect);
}

//-----------------------------------------------------------------------------
// Keep our text when fields are edited in the Inspector
//-----------------------------------------------------------------------------
void GuiTextEditColorCtrl::onStaticModified(const char* slotName,
   const char* newValue)
{
   // 1)  The Inspector re-sends the "text" field after every property edit.
   //     If that string is empty we *ignore* it so we don’t overwrite the
   //     live buffer with "" which would trigger the placeholder later on.
   //     If it’s non-empty we forward it to the base-class handler so the
   //     change is applied as normal.
   // 2)  All other fields are forwarded unchanged to the parent.
   //-------------------------------------------------------------------------
   if (dStricmp(slotName, "text") == 0)
   {
      // newValue can legitimately be NULL when the inspector just “touched”
      // the field without changing it.
      if (newValue && newValue[0])
         Parent::setText(newValue);        // apply user-supplied text
      // else: keep existing contents, do nothing
   }
   else
   {
      // default processing for everything else
      Parent::onStaticModified(slotName, newValue);
   }
}
