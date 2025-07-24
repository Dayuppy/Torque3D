//-----------------------------------------------------------------------------
// GuiRadialMenuCtrl.h
// Hierarchical radial menu control for Torque3D, with robust asset icon support
//-----------------------------------------------------------------------------

#ifndef _GUI_RADIAL_MENU_CTRL_H_
#define _GUI_RADIAL_MENU_CTRL_H_

#include "gui/core/guiControl.h"
#include "console/consoleTypes.h"
#include "sfx/sfxProfile.h"
#include "assets/assetPtr.h"
#include "T3D/assets/ImageAsset.h"
#include "gfx/gfxTextureHandle.h"

class GuiRadialMenuCtrl : public GuiControl
{
    typedef GuiControl Parent;
    DECLARE_CONOBJECT(GuiRadialMenuCtrl);

public:
    GuiRadialMenuCtrl();
    ~GuiRadialMenuCtrl() override;

    static void initPersistFields();

    bool onAdd() override;
    void onRender(Point2I offset, const RectI& updateRect) override;
    void onMouseMove(const GuiEvent& evt) override;
    void onMouseDown(const GuiEvent& evt) override;
    void onMouseUp(const GuiEvent& evt) override;
    bool onWake() override;
    void onSleep() override;

    //----- Script API -----
    /// Clear *all* entries and reset to empty wheel
    void clearItems();

    /// Insert an Item under path into mRootItems (with or without icon)
    void addItemToTree(const char* path, const char* name, const char* funcName, const char* funcArg);
    void addItemToTree(const char* path, const char* name, const char* funcName, const char* funcArg, const char* iconAssetId);

protected:
    struct Item
    {
        StringTableEntry   name;
        StringTableEntry   funcName;
        StringTableEntry   funcArg;
        Vector<Item>       children;
        bool               isBackButton;

        StringTableEntry        iconAssetId;           // Holds the assetId string ("" if no asset)
        AssetPtr<ImageAsset>    iconAsset;             // AssetPtr may be invalid
        GFXTexHandle            iconTexture;           // Loaded icon texture (may be invalid)

        // Default constructor
        Item()
            : name(StringTable->EmptyString()),
            funcName(StringTable->EmptyString()),
            funcArg(StringTable->EmptyString()),
            children(),
            isBackButton(false),
            iconAssetId(StringTable->EmptyString()),
            iconAsset(),
            iconTexture()
        {
        }

        // Copy constructor
        Item(const Item& other)
            : name(other.name),
            funcName(other.funcName),
            funcArg(other.funcArg),
            children(other.children),
            isBackButton(other.isBackButton),
            iconAssetId(other.iconAssetId),
            iconAsset(),
            iconTexture()
        {
            if (other.iconAsset.notNull() && other.iconAsset->getStatus() == ImageAsset::Ok)
            {
                iconAsset = other.iconAsset;
                iconTexture = other.iconTexture;
            }
            else
            {
                iconAsset.clear();
                iconTexture = GFXTexHandle();
            }
        }

        // Assignment operator
        Item& operator=(const Item& other)
        {
            if (this == &other)
                return *this;
            name = other.name;
            funcName = other.funcName;
            funcArg = other.funcArg;
            children = other.children;
            isBackButton = other.isBackButton;
            iconAssetId = other.iconAssetId;
            if (other.iconAsset.notNull() && other.iconAsset->getStatus() == ImageAsset::Ok)
            {
                iconAsset = other.iconAsset;
                iconTexture = other.iconTexture;
            }
            else
            {
                iconAsset.clear();
                iconTexture = GFXTexHandle();
            }
            return *this;
        }
    };

    // The full tree of items
    Vector<Item>     mRootItems;
    // The items currently shown (a subset of mRootItems or deeper)
    Vector<Item*>    mCurrentItems;
    // Index into mCurrentItems
    S32              mCurrentIndex;

    // Which segment the mouse is over
    S32              mHoveredIndex;

    // Stack of previous views (each view is a list of pointers to items)
    Vector< Vector<Item*> > mHistory;

    // A single reusable back‐button instance
    Item                    mBackItem;

    // Internal: Load icon fields for an item (if assetId given, may remain blank)
    static void  safeLoadIcon(Item& item, const char* assetId);
    // Internal: Get assetId from an item if its asset is valid, else ""
    static const char* _getItemIconAssetId(const Item& item);

    void centerCursor();

    // Visual parameters
    S32              mRadius;
    S32              mThickness;
    F32              mBaseIconSize;
    F32              mHoverScale;
    ColorI           mBackgroundColor;
    ColorI           mHighlightColor;

    // Sounds
    SimObjectPtr<SFXProfile> mHoverSound;
    SimObjectPtr<SFXProfile> mClickSound;

    // How many radial lines to draw per wedge (higher = smoother)
    static const U32 kWedgeSteps = 1;

    //— Rendering helpers —
    void drawWedgeFill(const Point2I& center,
        F32 radius,
        F32 startAng,
        F32 endAng,
        const ColorI& col);

    void renderItems(const Point2I& center);
    void renderBottomTextWithFrame(const Point2I& center);
    S32  hitTestSegment(const Point2I& local) const;

    //— Tree helpers —
    /// Reset mCurrentItems to the top‐level mRootItems
    void gatherCurrentLevel();
};

#endif // _GUI_RADIAL_MENU_CTRL_H_

static bool isValidAssetId(const char* assetId);
