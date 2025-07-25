//-----------------------------------------------------------------------------
// File: GuiBitmapItemExtended.cpp
//-----------------------------------------------------------------------------

#include "GuiBitmapItemExtended.h"
#include "console/engineAPI.h"
#include "gfx/gfxTextureProfile.h"


IMPLEMENT_CONOBJECT(GuiBitmapItemExtended);
ConsoleDocClass(GuiBitmapItemExtended,
   "@brief GUI control: square bitmap with optional rotation, tint, shadow, background, text overlay, and customizable font.\n"
   "@ingroup GuiControls"
);

//-----------------------------------------------------------------------------
// _setBitmapAsset
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

//-----------------------------------------------------------------------------
// _getBitmapAsset
//-----------------------------------------------------------------------------
const char* GuiBitmapItemExtended::_getBitmapAsset(void* obj, const char* data)
{
   GuiBitmapItemExtended* ctrl = static_cast<GuiBitmapItemExtended*>(obj);
   return ctrl->mBitmapAsset.notNull() ? ctrl->mBitmapAsset->getAssetId() : StringTable->EmptyString();
}

//-----------------------------------------------------------------------------
// onAssetRefresh
//-----------------------------------------------------------------------------
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
   : mBitmapAsset(nullptr), mBitmap(nullptr),
   mColor(255, 255, 255, 255), mAngle(0.0f),
   mDrawBackgroundRect(false), mBackgroundColor(0, 0, 0, 150),
   mBitmapShadow(false), mBitmapShadowColor(0, 0, 0, 128), mBitmapShadowOffset(2, 2),
   mTitle(nullptr), mBottomLeftText(nullptr), mBottomRightText(nullptr),
   mProgress(0.0f), mBarColor(0, 200, 0, 255), mTextColor(255, 255, 255, 255),
   mDrawTextFrame(false), mTextFrameColor(0, 0, 0, 150), mTextFramePadding(2),
   mFontName(StringTable->insert("Roboto Condensed")),
   mFontSize(18),
   mCustomFont(nullptr)
{
   mTitle = StringTable->insert("");
   mBottomLeftText = StringTable->insert("");
   mBottomRightText = StringTable->insert("");

   refreshFont();
}

//-----------------------------------------------------------------------------
// initPersistFields
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

   addGroup("Font");
   addField("fontName", TypeCaseString, Offset(mFontName, GuiBitmapItemExtended), "Font face name");
   addField("fontSize", TypeS32, Offset(mFontSize, GuiBitmapItemExtended), "Font size");
   endGroup("Font");
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

   // refresh font now
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
   // early out
   Point2I ext = getExtent();
   if (ext.x <= 0 || ext.y <= 0 || !mProfile) return;

   // square
   S32 size = ext.x < ext.y ? ext.x : ext.y;
   RectI bounds(offset, Point2I(size, size));
   GFXDrawUtil* du = GFX->getDrawUtil();

   // reset
   du->clearBitmapModulation();
   ColorI prevMod; du->getBitmapModulation(&prevMod);

   // shadow
   if (mBitmapShadow && mBitmap.isValid())
   {
      du->setBitmapModulation(mBitmapShadowColor);
      RectI sh = bounds; sh.point += mBitmapShadowOffset; sh.inset(2, 2);
      du->drawBitmapStretch(mBitmap, sh, GFXBitmapFlip_None, GFXTextureFilterLinear, false, mAngle);
      du->setBitmapModulation(prevMod);
   }

   // background
   if (mDrawBackgroundRect)
   {
      du->setBitmapModulation(mBackgroundColor);
      du->drawRoundedRect(4.0f, bounds, mBackgroundColor, 2.0f, mBackgroundColor);
      du->setBitmapModulation(prevMod);
   }

   // bitmap
   if (mBitmap.isValid())
   {
      du->setBitmapModulation(mColor);
      RectI tr = bounds; tr.inset(2, 2);
      du->drawBitmapStretch(mBitmap, tr, GFXBitmapFlip_None, GFXTextureFilterLinear, false, mAngle);
      du->setBitmapModulation(prevMod);
   }

   // overlay
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
         drawLabel(mTitle, bounds.point + Point2I((size - font->getStrWidth(mTitle)) / 2, 4));
      if (mBottomLeftText && mBottomLeftText[0])
         drawLabel(mBottomLeftText, bounds.point + Point2I(4, size - font->getHeight() - 8));
      if (mBottomRightText && mBottomRightText[0])
         drawLabel(mBottomRightText, bounds.point + Point2I(size - font->getStrWidth(mBottomRightText) - 4, size - font->getHeight() - 8));

      // progress
      S32 barH = 6, margin = 16;
      S32 barY = size - font->getHeight() - 8 + margin;
      RectI bg(bounds.point + Point2I(4, barY), Point2I(size - 8, barH));
      du->setBitmapModulation(prevMod);
      du->drawRoundedRect(2.0f, bg, ColorI(40, 40, 40, 150), 2.0f, ColorI(40, 40, 40, 150));
      if (mProgress > 0.0f)
      {
         RectI f = bg; f.extent.x = S32(f.extent.x * mProgress);
         du->setBitmapModulation(mBarColor);
         du->drawRectFill(f, mBarColor);
      }
   }

   du->setBitmapModulation(prevMod);
   renderChildControls(offset, updateRect);
}

void GuiBitmapItemExtended::onMouseDragged(const GuiEvent& event)
{
   Parent::onMouseDragged(event);
}

IMPLEMENT_CALLBACK(GuiBitmapItemExtended, onMouseDragged, void, (), (),
   "If #useMouseEvents is true, this is called when a left mouse button drag is detected, i.e. when the user "
   "pressed the left mouse button on the control and then moves the mouse over a certain distance threshold with "
   "the mouse button still pressed.");
