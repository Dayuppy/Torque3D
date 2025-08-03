//-----------------------------------------------------------------------------
// File: GuiBitmapItemExtended.cpp
//-----------------------------------------------------------------------------

#include "GuiBitmapItemExtended.h"
#include "console/engineAPI.h"
#include "gfx/gfxTextureProfile.h"
#include "gui/core/guiCanvas.h"

static const char* sForceGlowFieldName = "forceGlow";

IMPLEMENT_CONOBJECT(GuiBitmapItemExtended);
ConsoleDocClass(GuiBitmapItemExtended,
   "@brief GUI control: square bitmap with optional rotation, tint, shadow, background, text overlay, and customizable font.\n"
   "@ingroup GuiControls"
);

IMPLEMENT_CALLBACK(GuiBitmapItemExtended, onMouseDragged, void, (), (),
   "If #useMouseEvents is true, this is called when a left mouse button drag is detected, i.e. when the user "
   "pressed the left mouse button on the control and then moves the mouse over a certain distance threshold with "
   "the mouse button still pressed.");

IMPLEMENT_CALLBACK(GuiBitmapItemExtended, onMouseDown, void, (), (),
   "If #useMouseEvents is true, this is called when the left mouse button is pressed on an (active) "
   "button.");

IMPLEMENT_CALLBACK(GuiBitmapItemExtended, onMouseUp, void, (), (),
   "If #useMouseEvents is true, this is called when the left mouse button is release over an (active) "
   "button.\n\n"
   "@note To trigger actions, better use onClick() since onMouseUp() will also be called when the mouse was "
   "not originally pressed on the button.");

IMPLEMENT_CALLBACK(GuiBitmapItemExtended, onMouseEnter, void, (), (),
   "If #useMouseEvents is true, this is called when the mouse cursor moves over the button (only if the button "
   "is the front-most visible control, though).");

IMPLEMENT_CALLBACK(GuiBitmapItemExtended, onMouseLeave, void, (), (),
   "If #useMouseEvents is true, this is called when the mouse cursor moves off the button (only if the button "
   "had previously received an onMouseEvent() event).");

IMPLEMENT_CALLBACK(GuiBitmapItemExtended, onClick, void, (), (),
   "Called when the primary action of the button is triggered (e.g. by a left mouse click).");

IMPLEMENT_CALLBACK(GuiBitmapItemExtended, onRightClick, void, (), (),
   "Called when the right mouse button is clicked on the button.");

//-----------------------------------------------------------------------------
// Asset setters (same style as icon)
//-----------------------------------------------------------------------------
bool GuiBitmapItemExtended::_setBitmapFrameAsset(void* obj, const char* index, const char* data)
{
   GuiBitmapItemExtended* ctrl = static_cast<GuiBitmapItemExtended*>(obj);
   if (ctrl->mBitmapFrameAsset.notNull())
      ctrl->mBitmapFrameAsset->getChangedSignal().remove(ctrl, &GuiBitmapItemExtended::onFrameAssetRefresh);

   if (!data || !*data)
   {
      ctrl->mBitmapFrameAsset.clear();
      ctrl->mBitmapFrame = GFXTexHandle();
   }
   else
   {
      ctrl->mBitmapFrameAsset = AssetPtr<ImageAsset>(StringTable->insert(data));
      ctrl->mBitmapFrameAsset->getChangedSignal().notify(ctrl, &GuiBitmapItemExtended::onFrameAssetRefresh);
      ctrl->mBitmapFrame = ctrl->mBitmapFrameAsset->getTexture(&GFXDefaultGUIProfile);
   }
   ctrl->setUpdate();
   return false;
}
const char* GuiBitmapItemExtended::_getBitmapFrameAsset(void* obj, const char* data)
{
   GuiBitmapItemExtended* ctrl = static_cast<GuiBitmapItemExtended*>(obj);
   return ctrl->mBitmapFrameAsset.notNull() ? ctrl->mBitmapFrameAsset->getAssetId() : StringTable->EmptyString();
}

bool GuiBitmapItemExtended::_setBitmapGlowAsset(void* obj, const char* index, const char* data)
{
   GuiBitmapItemExtended* ctrl = static_cast<GuiBitmapItemExtended*>(obj);
   if (ctrl->mBitmapGlowAsset.notNull())
      ctrl->mBitmapGlowAsset->getChangedSignal().remove(ctrl, &GuiBitmapItemExtended::onGlowAssetRefresh);

   if (!data || !*data)
   {
      ctrl->mBitmapGlowAsset.clear();
      ctrl->mBitmapGlow = GFXTexHandle();
   }
   else
   {
      ctrl->mBitmapGlowAsset = AssetPtr<ImageAsset>(StringTable->insert(data));
      ctrl->mBitmapGlowAsset->getChangedSignal().notify(ctrl, &GuiBitmapItemExtended::onGlowAssetRefresh);
      ctrl->mBitmapGlow = ctrl->mBitmapGlowAsset->getTexture(&GFXDefaultGUIProfile);
   }
   ctrl->setUpdate();
   return false;
}
const char* GuiBitmapItemExtended::_getBitmapGlowAsset(void* obj, const char* data)
{
   GuiBitmapItemExtended* ctrl = static_cast<GuiBitmapItemExtended*>(obj);
   return ctrl->mBitmapGlowAsset.notNull() ? ctrl->mBitmapGlowAsset->getAssetId() : StringTable->EmptyString();
}

//-----------------------------------------------------------------------------
// DefineEngineMethod to force glow from script
//-----------------------------------------------------------------------------
DefineEngineMethod(GuiBitmapItemExtended, setForceGlow, void, (bool enabled), ,
   "@brief Force the glow outline to be on, even if not hovered or selected.\n"
   "This allows the control to be highlighted via script.\n"
   "@param enabled True to force glow, false to return to normal behavior."
)
{
   object->setForceGlow(enabled);
}
void GuiBitmapItemExtended::setForceGlow(bool enabled)
{
   mForceGlow = enabled;
   setUpdate();
}

//-----------------------------------------------------------------------------
// Asset refresh for frame and glow bitmaps
//-----------------------------------------------------------------------------
void GuiBitmapItemExtended::onFrameAssetRefresh()
{
   if (mBitmapFrameAsset.notNull() && mBitmapFrameAsset->getStatus() == ImageAsset::Ok)
      mBitmapFrame = mBitmapFrameAsset->getTexture(&GFXDefaultGUIProfile);
   else
      mBitmapFrame = GFXTexHandle();
   setUpdate();
}
void GuiBitmapItemExtended::onGlowAssetRefresh()
{
   if (mBitmapGlowAsset.notNull() && mBitmapGlowAsset->getStatus() == ImageAsset::Ok)
      mBitmapGlow = mBitmapGlowAsset->getTexture(&GFXDefaultGUIProfile);
   else
      mBitmapGlow = GFXTexHandle();
   setUpdate();
}

//-----------------------------------------------------------------------------
// Icon asset pattern
//-----------------------------------------------------------------------------
bool GuiBitmapItemExtended::_setBitmapAsset(void* obj, const char* index, const char* data)
{
   GuiBitmapItemExtended* ctrl = static_cast<GuiBitmapItemExtended*>(obj);
   if (ctrl->mBitmapAsset.notNull())
      ctrl->mBitmapAsset->getChangedSignal().remove(ctrl, &GuiBitmapItemExtended::onAssetRefresh);

   if (!data || !*data)
   {
      ctrl->mBitmapAsset.clear();
      ctrl->mBitmap = GFXTexHandle();
   }
   else
   {
      ctrl->mBitmapAsset = AssetPtr<ImageAsset>(StringTable->insert(data));
      ctrl->mBitmapAsset->getChangedSignal().notify(ctrl, &GuiBitmapItemExtended::onAssetRefresh);
      ctrl->mBitmap = ctrl->mBitmapAsset->getTexture(&GFXDefaultGUIProfile);
   }
   ctrl->setUpdate();
   return false;
}
const char* GuiBitmapItemExtended::_getBitmapAsset(void* obj, const char* data)
{
   GuiBitmapItemExtended* ctrl = static_cast<GuiBitmapItemExtended*>(obj);
   return ctrl->mBitmapAsset.notNull() ? ctrl->mBitmapAsset->getAssetId() : StringTable->EmptyString();
}
void GuiBitmapItemExtended::onAssetRefresh()
{
   if (mBitmapAsset.notNull() && mBitmapAsset->getStatus() == ImageAsset::Ok)
      mBitmap = mBitmapAsset->getTexture(&GFXDefaultGUIProfile);
   else
      mBitmap = GFXTexHandle();
   setUpdate();
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
GuiBitmapItemExtended::GuiBitmapItemExtended()
   : mBitmapAsset(nullptr),
   mBitmap(nullptr),
   mBitmapFrameAsset(nullptr),
   mBitmapFrame(nullptr),
   mBitmapGlowAsset(nullptr),
   mBitmapGlow(nullptr),
   mUseBitmapFrame(false),
   mUseBitmapGlow(false),
   mColor(255, 255, 255, 255),
   mAngle(0.0f),
   mDrawBackgroundRect(true),
   mBackgroundColor(255, 255, 255, 255),
   mBitmapShadow(true),
   mBitmapShadowColor(0, 0, 0, 192),
   mBitmapShadowOffset(2, 2),
   mTitle(nullptr),
   mBottomLeftText(nullptr),
   mBottomRightText(nullptr),
   mProgress(0.0f),
   mBarColor(0, 200, 0, 255),
   mTextColor(255, 255, 255, 255),
   mDrawTextFrame(true),
   mTextFrameColor(0, 0, 0, 192),
   mTextFramePadding(1),
   mFontName(StringTable->insert("Roboto Condensed")),
   mFontSize(20),
   mCustomFont(nullptr),
   mGlowEnabled(true),
   mGlowColor(255, 255, 0, 255),
   mGlowThickness(4),
   mHovering(false),
   mForceGlow(false)
{
   mTitle = StringTable->insert("");
   mBottomLeftText = StringTable->insert("");
   mBottomRightText = StringTable->insert("");

   refreshFont();
}

//-----------------------------------------------------------------------------
// Persist fields for assets
//-----------------------------------------------------------------------------
void GuiBitmapItemExtended::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("BitmapItem");
   addProtectedField("bitmapAsset", TypeImageAssetPtr, Offset(mBitmapAsset, GuiBitmapItemExtended),
      &GuiBitmapItemExtended::_setBitmapAsset, &GuiBitmapItemExtended::_getBitmapAsset, "Image asset ID");
   addField("color", TypeColorI, Offset(mColor, GuiBitmapItemExtended), "Bitmap tint & alpha");
   addField("angle", TypeF32, Offset(mAngle, GuiBitmapItemExtended), "Rotation angle");
   addField("drawBackground", TypeBool, Offset(mDrawBackgroundRect, GuiBitmapItemExtended), "Draw background rect");
   addField("backgroundColor", TypeColorI, Offset(mBackgroundColor, GuiBitmapItemExtended), "Background fill color");
   addField("bitmapShadow", TypeBool, Offset(mBitmapShadow, GuiBitmapItemExtended), "Enable drop shadow");
   addField("bitmapShadowColor", TypeColorI, Offset(mBitmapShadowColor, GuiBitmapItemExtended), "Shadow color");
   addField("bitmapShadowOffset", TypePoint2I, Offset(mBitmapShadowOffset, GuiBitmapItemExtended), "Shadow offset");

   addProtectedField("bitmapFrameAsset", TypeImageAssetPtr, Offset(mBitmapFrameAsset, GuiBitmapItemExtended),
      &GuiBitmapItemExtended::_setBitmapFrameAsset, &GuiBitmapItemExtended::_getBitmapFrameAsset, "Image asset ID for frame");
   addProtectedField("bitmapGlowAsset", TypeImageAssetPtr, Offset(mBitmapGlowAsset, GuiBitmapItemExtended),
      &GuiBitmapItemExtended::_setBitmapGlowAsset, &GuiBitmapItemExtended::_getBitmapGlowAsset, "Image asset ID for glow");
   addField("useBitmapFrame", TypeBool, Offset(mUseBitmapFrame, GuiBitmapItemExtended), "Use bitmap frame asset");
   addField("useBitmapGlow", TypeBool, Offset(mUseBitmapGlow, GuiBitmapItemExtended), "Use bitmap glow asset");
   endGroup("BitmapItem");

   addGroup("Overlay");
   addField("title", TypeString, Offset(mTitle, GuiBitmapItemExtended), "Title text");
   addField("bottomLeft", TypeString, Offset(mBottomLeftText, GuiBitmapItemExtended), "Bottom-left label");
   addField("bottomRight", TypeString, Offset(mBottomRightText, GuiBitmapItemExtended), "Bottom-right label");
   addField("progress", TypeF32, Offset(mProgress, GuiBitmapItemExtended), "Progress [0..1]");
   addField("barColor", TypeColorI, Offset(mBarColor, GuiBitmapItemExtended), "Progress bar color");
   addField("textColor", TypeColorI, Offset(mTextColor, GuiBitmapItemExtended), "Text color");
   addField("drawTextFrame", TypeBool, Offset(mDrawTextFrame, GuiBitmapItemExtended), "Draw text frame");
   addField("textFrameColor", TypeColorI, Offset(mTextFrameColor, GuiBitmapItemExtended), "Text frame color");
   addField("textFramePadding", TypeS32, Offset(mTextFramePadding, GuiBitmapItemExtended), "Text frame padding");
   endGroup("Overlay");

   addGroup("Glow");
   addField("glowEnabled", TypeBool, Offset(mGlowEnabled, GuiBitmapItemExtended),
      "Enable glow outline on hover or focus");
   addField("glowColor", TypeColorI, Offset(mGlowColor, GuiBitmapItemExtended),
      "Glow outline color");
   addField("glowThickness", TypeS32, Offset(mGlowThickness, GuiBitmapItemExtended),
      "Glow outline thickness in pixels");
   addField(sForceGlowFieldName, TypeBool, Offset(mForceGlow, GuiBitmapItemExtended),
      "Force the glow outline even if not hovered or selected");
   endGroup("Glow");

   addGroup("Font");
   addField("fontName", TypeCaseString, Offset(mFontName, GuiBitmapItemExtended), "Font face name");
   addField("fontSize", TypeS32, Offset(mFontSize, GuiBitmapItemExtended), "Font size");
   endGroup("Font");
}

//-----------------------------------------------------------------------------
// Mouse events
//-----------------------------------------------------------------------------
void GuiBitmapItemExtended::onAction()
{
   if (!mActive)
      return;
   onClick_callback();
   Parent::onAction();
}
void GuiBitmapItemExtended::onRightMouseUp(const GuiEvent& event)
{
   if (!mActive)
      return;
   onRightClick_callback();
   Parent::onRightMouseUp(event);
}


void GuiBitmapItemExtended::onMouseDragged(const GuiEvent& event)
{
   onMouseDragged_callback();
   Parent::onMouseDragged(event);
}
void GuiBitmapItemExtended::onMouseDown(const GuiEvent& event)
{
   if (!mActive)
      return;
  onMouseDown_callback();
}
void GuiBitmapItemExtended::onMouseUp(const GuiEvent& event)
{
   Parent::onMouseUp(event);
   onMouseUp_callback();
}
void GuiBitmapItemExtended::onMouseEnter(const GuiEvent& event)
{
   Parent::onMouseEnter(event);
   mHovering = true;
   setUpdate();
   onMouseEnter_callback();
}
void GuiBitmapItemExtended::onMouseLeave(const GuiEvent& event)
{
   Parent::onMouseLeave(event);
   mHovering = false;
   setUpdate();
   onMouseLeave_callback();
}
void GuiBitmapItemExtended::onGainFirstResponder()
{
   Parent::onGainFirstResponder();
   setUpdate();
}
void GuiBitmapItemExtended::onLoseFirstResponder()
{
   Parent::onLoseFirstResponder();
   setUpdate();
}

//-----------------------------------------------------------------------------
// onAdd
//-----------------------------------------------------------------------------
bool GuiBitmapItemExtended::onAdd()
{
   if (!Parent::onAdd()) return false;
   return true;
}

//-----------------------------------------------------------------------------
// onWake
//-----------------------------------------------------------------------------
bool GuiBitmapItemExtended::onWake()
{
   if (!Parent::onWake()) return false;
   setActive(true);

   // load bitmap
   if (mBitmapAsset.notNull() && mBitmapAsset->getStatus() == ImageAsset::Ok)
      mBitmap = mBitmapAsset->getTexture(&GFXDefaultGUIProfile);

   if (mBitmapFrameAsset.notNull() && mBitmapFrameAsset->getStatus() == ImageAsset::Ok)
      mBitmapFrame = mBitmapFrameAsset->getTexture(&GFXDefaultGUIProfile);

   if (mBitmapGlowAsset.notNull() && mBitmapGlowAsset->getStatus() == ImageAsset::Ok)
      mBitmapGlow = mBitmapGlowAsset->getTexture(&GFXDefaultGUIProfile);

   refreshFont();
   return true;
}

//-----------------------------------------------------------------------------
// refreshFont
//-----------------------------------------------------------------------------
void GuiBitmapItemExtended::refreshFont()
{
   if (mFontName && mFontName[0] && mFontSize > 0)
   {
      Resource<GFont> f = GFont::create(mFontName, mFontSize);
      if (f) mCustomFont = f;
      else  Con::warnf("GuiBitmapItemExtended::refreshFont - could not create font '%s' size %d", mFontName, mFontSize);
   }
   else
      mCustomFont = nullptr;
}

//-----------------------------------------------------------------------------
// onRender
//-----------------------------------------------------------------------------
void GuiBitmapItemExtended::onRender(Point2I offset, const RectI& updateRect)
{
   Point2I ext = getExtent();
   if (ext.x <= 0 || ext.y <= 0 || !mProfile) return;

   S32 framePad = mGlowThickness;
   S32 frameSize = (ext.x < ext.y ? ext.x : ext.y);
   RectI frameRect(offset, Point2I(frameSize, frameSize));
   RectI contentRect = frameRect;
   contentRect.inset(framePad, framePad);

   GFXDrawUtil* du = GFX->getDrawUtil();
   du->clearBitmapModulation();
   ColorI prevMod; du->getBitmapModulation(&prevMod);

   // 1. Draw frame (bitmap or drawn)
   if (mDrawBackgroundRect)
   {
      if (mUseBitmapFrame && mBitmapFrame.isValid())
      {
         du->setBitmapModulation(mBackgroundColor);
         du->drawBitmapStretch(mBitmapFrame, frameRect, GFXBitmapFlip_None, GFXTextureFilterLinear, false);
         du->setBitmapModulation(prevMod);
      }
      else
      {
         du->setBitmapModulation(ColorI(0, 0, 0, 0));
         du->drawRoundedRect(4.0f, frameRect, ColorI(0, 0, 0, 0), 0.0f, ColorI(0, 0, 0, 0));
         du->setBitmapModulation(prevMod);
      }
   }

   // 2. Draw shadow
   if (mBitmapShadow && mBitmap.isValid())
   {
      du->setBitmapModulation(mBitmapShadowColor);
      RectI sh = contentRect; sh.point += mBitmapShadowOffset; sh.inset(2, 2);
      du->drawBitmapStretch(mBitmap, sh, GFXBitmapFlip_None, GFXTextureFilterLinear, false, mAngle);
      du->setBitmapModulation(prevMod);
   }

   // 3. Draw background rectangle (inside content area)
   if (mDrawBackgroundRect && (!mUseBitmapFrame || !mBitmapFrame.isValid()))
   {
      du->setBitmapModulation(mBackgroundColor);
      du->drawRoundedRect(4.0f, contentRect, mBackgroundColor, 2.0f, mBackgroundColor);
      du->setBitmapModulation(prevMod);
   }

   // 4. Draw bitmap (inside content area)
   if (mBitmap.isValid())
   {
      du->setBitmapModulation(mColor);
      RectI tr = contentRect; tr.inset(2, 2);
      du->drawBitmapStretch(mBitmap, tr, GFXBitmapFlip_None, GFXTextureFilterLinear, false, mAngle);
      du->setBitmapModulation(prevMod);
   }

   // 5. Overlay text (inside content area)
   GFont* font = mCustomFont ? mCustomFont : mProfile->mFont;
   if (font)
   {
      auto drawLabel = [&](const char* txt, Point2I pos)
         {
            S32 w = font->getStrWidth(txt), h = font->getHeight();
            if (mDrawTextFrame)
            {
               RectI fr(pos.x - mTextFramePadding, pos.y - mTextFramePadding, w + 2 * mTextFramePadding, h + 2 * mTextFramePadding);
               du->setBitmapModulation(mTextFrameColor);
               du->drawRoundedRect(4.0f, fr, mTextFrameColor, 2.0f, mTextFrameColor);
            }
            du->setBitmapModulation(mTextColor);
            du->drawText(font, pos, txt);
            du->setBitmapModulation(prevMod);
         };

      if (mTitle && mTitle[0])
         drawLabel(mTitle, contentRect.point + Point2I((contentRect.extent.x - font->getStrWidth(mTitle)) / 2, 4));
      if (mBottomLeftText && mBottomLeftText[0])
         drawLabel(mBottomLeftText, contentRect.point + Point2I(4, contentRect.extent.y - font->getHeight() - 8));
      if (mBottomRightText && mBottomRightText[0])
         drawLabel(mBottomRightText, contentRect.point + Point2I(contentRect.extent.x - font->getStrWidth(mBottomRightText) - 4, contentRect.extent.y - font->getHeight() - 8));

      // progress bar
      S32 barH = 6, margin = 16;
      S32 barY = contentRect.extent.y - font->getHeight() - 8 + margin;
      RectI bg(contentRect.point + Point2I(4, barY), Point2I(contentRect.extent.x - 8, barH));
      du->setBitmapModulation(prevMod);
      du->drawRoundedRect(2.0f, bg, ColorI(40, 40, 40, 150), 2.0f, ColorI(40, 40, 40, 150));
      if (mProgress > 0.0f)
      {
         RectI f = bg; f.extent.x = S32(f.extent.x * mProgress);
         du->setBitmapModulation(mBarColor);
         du->drawRectFill(f, mBarColor);
      }
   }

   // 6. Glow border (bitmap or drawn)
   bool mouseEnabled = true;
   GuiCanvas* canvas = dynamic_cast<GuiCanvas*>(getRoot());
   if (canvas)
      mouseEnabled = canvas->isCursorON();

   if (mGlowEnabled && ((mouseEnabled && (mHovering || isFirstResponder())) || mForceGlow))
   {
      ColorI savedMod;
      du->getBitmapModulation(&savedMod);
      du->setBitmapModulation(mGlowColor);

      if (mUseBitmapGlow && mBitmapGlow.isValid())
      {
         du->drawBitmapStretch(mBitmapGlow, frameRect, GFXBitmapFlip_None, GFXTextureFilterLinear, false);
         du->setBitmapModulation(savedMod);
      }
      else
      {
         du->drawRoundedRect(
            4.0f,
            frameRect,
            ColorI(0, 0, 0, 0),
            (F32)(2 * mGlowThickness),
            mGlowColor
         );
         du->setBitmapModulation(savedMod);
      }
   }

   du->setBitmapModulation(prevMod);
   renderChildControls(offset, updateRect);
}
