#include "guiRadialCompassControl.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/gameBase/gameBase.h"

IMPLEMENT_CONOBJECT(GuiRadialCompassControl);

GuiRadialCompassControl::GuiRadialCompassControl()
{
    // Default places full compass just inside top-right
    mCenterOffset.set(-50, 50);
    mRadius = 40.0f;
    mTickLength = 8.0f;
    mTextDistance = 12.0f;
    mHashmarkColor.set(255, 255, 255, 255);
    mNorthColor.set(255, 255, 0, 255);
    mNeedleColor.set(255, 0, 0, 255);
    mDegreeColor.set(255, 0, 0, 255);
    mPrevYaw = 0.0f;
}

void GuiRadialCompassControl::initPersistFields()
{
    Parent::initPersistFields();
    addField("centerOffset", TypePoint2F, Offset(mCenterOffset, GuiRadialCompassControl));
    addField("radius", TypeF32, Offset(mRadius, GuiRadialCompassControl));
    addField("tickLength", TypeF32, Offset(mTickLength, GuiRadialCompassControl));
    addField("textDistance", TypeF32, Offset(mTextDistance, GuiRadialCompassControl));
    addField("hashmarkColor", TypeColorI, Offset(mHashmarkColor, GuiRadialCompassControl));
    addField("northColor", TypeColorI, Offset(mNorthColor, GuiRadialCompassControl));
    addField("needleColor", TypeColorI, Offset(mNeedleColor, GuiRadialCompassControl));
    addField("degreeColor", TypeColorI, Offset(mDegreeColor, GuiRadialCompassControl));
}

void GuiRadialCompassControl::onRender(Point2I offset, const RectI& updateRect)
{
    GameConnection* conn = GameConnection::getConnectionToServer();
    if (!conn) return;
    GameBase* control = dynamic_cast<GameBase*>(conn->getControlObject());
    if (!control) return;

    // Calculate yaw
    VectorF fwd = control->getTransform().getForwardVector();
    F32 yaw = mRadToDeg(mAtan2(fwd.x, fwd.y));
    yaw = mFmod(yaw + 360.0f, 360.0f);
    F32 diff = mFmod(yaw - mPrevYaw + 540.0f, 360.0f) - 180.0f;
    F32 smoothedYaw = mFmod(mPrevYaw + diff * 0.2f + 360.0f, 360.0f);
    mPrevYaw = smoothedYaw;

    GFXDrawUtil* drawer = GFX->getDrawUtil();
    GFont* font = mProfile->mFont;
    if (!font) return;

    // Place center at top-right corner plus offset
    Point2I ext = getExtent();
    Point2I center = offset + Point2I(ext.x, 0) + Point2I((S32)mCenterOffset.x, (S32)mCenterOffset.y);

    // Circular border
    const S32 segments = 64;
    for (S32 i = 0; i < segments; ++i)
    {
        F32 a0 = M_PI_F * 2 * (i / (F32)segments);
        F32 a1 = M_PI_F * 2 * ((i + 1) / (F32)segments);
        Point2I p0((S32)(center.x + mCos(a0) * mRadius), (S32)(center.y + mSin(a0) * mRadius));
        Point2I p1((S32)(center.x + mCos(a1) * mRadius), (S32)(center.y + mSin(a1) * mRadius));
        drawer->drawLine(p0, p1, mHashmarkColor);
    }

    // Cardinal ticks & labels
    for (S32 a = 0; a < 360; a += 45)
    {
        F32 ang = mDegToRad(mFmod((a - smoothedYaw - 90 + 360), 360));
        F32 cosA = mCos(ang), sinA = mSin(ang);
        Point2I in(center.x + cosA * (mRadius - mTickLength), center.y + sinA * (mRadius - mTickLength));
        Point2I out(center.x + cosA * mRadius, center.y + sinA * mRadius);
        drawer->drawLine(in, out, (a == 0) ? mNorthColor : mHashmarkColor);

        const char* lbl = "";
        switch (a)
        {
        case 0:   lbl = "N"; break;
        case 45:  lbl = "NE"; break;
        case 90:  lbl = "E"; break;
        case 135: lbl = "SE"; break;
        case 180: lbl = "S"; break;
        case 225: lbl = "SW"; break;
        case 270: lbl = "W"; break;
        case 315: lbl = "NW"; break;
        }
        Point2I lp(center.x + cosA * (mRadius + mTextDistance) - 6,
            center.y + sinA * (mRadius + mTextDistance) - 6);
        drawer->drawTextShadowed(font, lp, Point2I(2, 2), lbl, (a == 0) ? mNorthColor : mHashmarkColor, ColorI(0, 0, 0, 255));
    }

    // Needle & degrees
    drawer->drawLine(center + Point2I(0, -20), center + Point2I(0, -10), mNeedleColor);
    char buf[8]; dSprintf(buf, sizeof(buf), "%d%c", (S32)smoothedYaw, 176);
    drawer->drawTextShadowed(font, center + Point2I(-12, 20), Point2I(2, 2), buf, mDegreeColor, ColorI(0, 0, 0, 255));
}
