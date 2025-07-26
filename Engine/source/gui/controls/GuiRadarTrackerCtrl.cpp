#include "guiRadarTrackerCtrl.h"
#include "gfx/gfxDrawUtil.h"
#include "math/mMathFn.h"
#include "platform/platformTimer.h"
#include "console/engineAPI.h"
#include "math/mathUtils.h"

IMPLEMENT_CONOBJECT(GuiRadarTrackerCtrl);

GuiRadarTrackerCtrl::GuiRadarTrackerCtrl()
{
    mZoom = 1.0f;
    mScanAngle = 0.0f;
    mScanSpeed = 90.0f;
    mUpdateRateSec = 0.25f;
    mUpdateTimer = 0.0f;
    mLastRenderTime = Platform::getRealMilliseconds();
    mRadarMode = Pulse;
    mPulseSpeed = 45.0f;
    mCenterObject = nullptr;
    mSweepTrailArc = 45.0f;
    mSweepTrailSteps = 12;
    // trail stays for one full second by default
    mSweepFadeTime = 0.20f;
    mSweepHistory.clear();
    mMaxRange = 256.0f;
}

bool GuiRadarTrackerCtrl::onWake()
{
   if (!Parent::onWake())
      return false;
   setProcessTicks(true);

   // load a blocky console font at size 12
   mConsoleFont = GFont::create("Terminal", 12);
   if (!mConsoleFont)
      Con::warnf("GuiRadarTrackerCtrl: failed to load console font 'Terminal', falling back.");

   return true;
}

void GuiRadarTrackerCtrl::onSleep()
{
    setProcessTicks(false);
    Parent::onSleep();
}

void GuiRadarTrackerCtrl::onPreRender()
{
    U32 now = Platform::getRealMilliseconds();
    F32 dt = (now - mLastRenderTime) / 1000.0f;

    // advance sweep
    mScanAngle = mFmod(mScanAngle + mScanSpeed * dt, 360.0f);

    // record history
    mSweepHistory.push_back({ mScanAngle, 0.0f });

    // age & prune
    for (S32 i = 0; i < (S32)mSweepHistory.size(); )
    {
        mSweepHistory[i].age += dt;
        if (mSweepHistory[i].age >= mSweepFadeTime)
            mSweepHistory.erase(i);
        else
            ++i;
    }

    // pulse logic...
    if (mRadarMode == Pulse)
    {
        mPulseRing.radius += mPulseSpeed * dt;
        mPulseRing.alpha -= dt * 0.4f;
        mPulseRing.alpha = mClampF(mPulseRing.alpha, 0.0f, 1.0f);
        F32 maxR = getMin(getBounds().extent.x, getBounds().extent.y) * 0.5f;
        if (mPulseRing.radius > maxR)
        {
            mPulseRing.radius = 0.0f;
            mPulseRing.alpha = 1.0f;
        }
    }

    // update tracked objects
    mUpdateTimer += dt;
    if (mUpdateTimer >= mUpdateRateSec)
    {
        mUpdateTimer = 0.0f;
        updateTrackedObjects();
    }

    // fade blips but never below 48/255
    const F32 minAlpha = 48.0f / 255.0f;
    for (RadarContact& c : mContacts)
    {
        c.alpha = mClampF(c.alpha - dt * 0.5f, minAlpha, 1.0f);
    }

    mLastRenderTime = now;
}

void GuiRadarTrackerCtrl::renderPulse(const RectI& bounds, const Point2I& origin)
{
    if (mRadarMode != Pulse || mPulseRing.alpha <= 0.01f)
        return;

    GFXDrawUtil* drawer = GFX->getDrawUtil();

    F32 r = mPulseRing.radius;
    F32 fadeAlpha = mClampF(mPulseRing.alpha, 0.0f, 1.0f);

    ColorI fillColor(0, 0, 0, 0); // transparent center
    ColorI borderColor(0, 255, 0, U8(120 * fadeAlpha));

    Point2F upperLeft(origin.x - r, origin.y - r);
    Point2F lowerRight(origin.x + r, origin.y + r);

    drawer->drawCircleFill(
        upperLeft,
        lowerRight,
        fillColor,
        r,             // this radius must match the size
        1.5f,          // border size
        borderColor
    );
}

void GuiRadarTrackerCtrl::onRender(Point2I offset, const RectI& updateRect)
{
   RectI bounds = getBounds();
   GFXDrawUtil* drawer = GFX->getDrawUtil();

   // compute center, pixel radius and corresponding world range
   Point2I origin = bounds.point + bounds.extent / 2;
   F32     pixelRadius = getMin(bounds.extent.x, bounds.extent.y) * 0.5f;
   F32     worldMax = pixelRadius * mZoom;

   // --- Restore circular background fill (50% opaque black) ---
   Point2F ul(origin.x - pixelRadius, origin.y - pixelRadius);
   Point2F lr(origin.x + pixelRadius, origin.y + pixelRadius);
   drawer->drawCircleFill(
      ul,
      lr,
      ColorI(0, 0, 0, 128),  // fill color
      pixelRadius,           // radius must match pixelRadius
      0.0f,                  // no border
      ColorI(0, 0, 0, 0)     // border color (unused)
   );

   // draw grid and sweep/pulse
   renderGrid(bounds, origin);
   if (mRadarMode == Pulse)
      renderPulse(bounds, origin);
   else
      renderSweep(bounds, origin);

   // draw blips with max‐range clamping, brackets, and distances
   renderBlips(bounds, origin, pixelRadius, worldMax);

   // any child controls on top
   renderChildControls(offset, updateRect);
}


void GuiRadarTrackerCtrl::renderGrid(const RectI& bounds, const Point2I& origin)
{
    GFXDrawUtil* drawer = GFX->getDrawUtil();
    ColorI gridColor(80, 255, 80, 40);
    F32 maxRadius = getMin(bounds.extent.x, bounds.extent.y) / 2.0f;

    // Rings scale inversely with zoom
    U32 rings = (U32)(maxRadius / (10.0f / mZoom));
    if (rings < 1) rings = 1;

    for (U32 i = 1; i <= rings; i++)
    {
        F32 r = i * (10.0f / mZoom);
        drawer->drawCircleFill(
            Point2F(origin.x - r, origin.y - r),
            Point2F(origin.x + r, origin.y + r),
            ColorI(0, 0, 0, 0),
            r,
            1.0f,
            gridColor
        );
    }

    // Radial spokes
    for (U32 a = 0; a < 360; a += 15)
    {
        F32 rad = mDegToRad((F32)a);
        Point2F dir(mCos(rad), -mSin(rad));
        Point2I p1(origin.x + dir.x * maxRadius, origin.y + dir.y * maxRadius);
        drawer->drawLine(origin, p1, gridColor);
    }
}

void GuiRadarTrackerCtrl::renderSweep(const RectI& bounds, const Point2I& origin)
{
    if (mRadarMode != Sweep || !mCenterObject)
        return;

    GFXDrawUtil* drawer = GFX->getDrawUtil();
    F32 radius = getMin(bounds.extent.x, bounds.extent.y) * 0.5f;

    // draw a single persistent sweep line for each history mark
    for (const SweepMark& mark : mSweepHistory)
    {
        F32 angleRad = mDegToRad(mark.angle);
        Point2F dir(mCos(angleRad), mSin(angleRad));  // radar‐space (+Y forward)

        Point2I tip(
            origin.x + S32(dir.x * radius),
            origin.y - S32(dir.y * radius)  // flip Y for screen coords
        );

        F32 fade = 1.0f - (mark.age / mSweepFadeTime);
        U8  alpha = U8(mClampF(fade, 0.0f, 1.0f) * 255);

        drawer->drawLine(origin, tip, ColorI(0, 255, 0, alpha));
    }
}

void GuiRadarTrackerCtrl::renderBlips(const RectI& bounds,
   const Point2I& origin,
   F32 pixelRadius,
   F32 worldMax)
{
   if (!mCenterObject)
      return;

   GFXDrawUtil* drawer = GFX->getDrawUtil();
   // Use the profile font (fallback) to guarantee text renders
   GFont* font = mProfile->mFont;
   Point3F center = mCenterObject->getPosition();
   MatrixF xf = mCenterObject->getTransform();

   // Precompute sweep‐arc bounds
   F32 halfArc = mSweepTrailArc * 0.5f;
   F32 sweepStart = mFmod(mScanAngle - halfArc + 360.0f, 360.0f);
   F32 sweepEnd = mFmod(mScanAngle + halfArc, 360.0f);

   const U8  minAlphaU8 = 48;
   const F32 pulseMargin = 8.0f;

   for (const RadarContact& c : mContacts)
   {
      // 1) Transform world→radar‐space (pixels)
      Point2F radarPos = worldToRadar(c.worldPos, center, xf);
      F32     worldDist = radarPos.len() * mZoom;
      if (worldDist < 0.01f)
         continue;

      // 2) Clamp beyond max range to the edge
      if (worldDist > worldMax)
      {
         radarPos.normalizeSafe();
         radarPos *= pixelRadius;
      }

      // 3) Base alpha from onPreRender (already ≥48/255)
      U8 alpha = U8(c.alpha * 255.0f);

      // 4) Highlight if currently scanned
      if (mRadarMode == Sweep)
      {
         F32 localAngle = mFmod(
            mRadToDeg(mAtan2(radarPos.y, radarPos.x)) + 360.0f,
            360.0f
         );
         bool inArc = (sweepStart < sweepEnd)
            ? (localAngle >= sweepStart && localAngle <= sweepEnd)
            : (localAngle >= sweepStart || localAngle <= sweepEnd);
         if (inArc) alpha = 255;
      }
      else // Pulse
      {
         bool inPulse = (worldDist >= (mPulseRing.radius - pulseMargin) &&
            worldDist <= (mPulseRing.radius + pulseMargin));
         if (inPulse) alpha = 255;
      }

      // 5) Enforce floor
      alpha = getMax(alpha, minAlphaU8);

      // 6) Compute screen coords
      Point2I screen(
         origin.x + S32(radarPos.x),
         origin.y - S32(radarPos.y)
      );

      // 7) Draw brackets
      ColorI bracketColor(0, 160, 0, alpha);
      const S32 b = 3;
      drawer->drawLine(screen + Point2I(-b, -b), screen + Point2I(-b / 2, -b), bracketColor);
      drawer->drawLine(screen + Point2I(-b, b), screen + Point2I(-b / 2, b), bracketColor);
      drawer->drawLine(screen + Point2I(b, -b), screen + Point2I(b / 2, -b), bracketColor);
      drawer->drawLine(screen + Point2I(b, b), screen + Point2I(b / 2, b), bracketColor);
      drawer->drawLine(screen + Point2I(-b, -b), screen + Point2I(-b, -b / 2), bracketColor);
      drawer->drawLine(screen + Point2I(b, -b), screen + Point2I(b, -b / 2), bracketColor);
      drawer->drawLine(screen + Point2I(-b, b), screen + Point2I(-b, b / 2), bracketColor);
      drawer->drawLine(screen + Point2I(b, b), screen + Point2I(b, b / 2), bracketColor);

      // 8) Draw the center dot
      drawer->drawRectFill(
         RectI(screen - Point2I(1, 1), Point2I(2, 2)),
         ColorI(0, 255, 0, alpha)
      );

      // 9) draw distance text in crisp console font
      char buf[16];
      dSprintf(buf, sizeof(buf), "%dm", S32(worldDist));
      Point2I textPos(screen.x + b + 2, screen.y - font->getHeight() / 2);

      // outline black, fill bright CRT green
      ColorI fillColor(0, 255, 0, alpha);
      ColorI outlineColor(0, 0, 0, alpha);

      drawer->drawTextOutlined(
         font,
         textPos,
         buf,
         fillColor,
         outlineColor,
         1  // outline thickness
      );
   }
}

void GuiRadarTrackerCtrl::updateTrackedObjects()
{
    if (!mCenterObject)
        return;

    Point3F center = mCenterObject->getPosition();
    MatrixF xf = mCenterObject->getTransform();

    // Precompute sweep bounds in radar space
    F32 halfArc = mSweepTrailArc * 0.5f;
    F32 sweepStart = mFmod(mScanAngle - halfArc + 360.0f, 360.0f);
    F32 sweepEnd = mFmod(mScanAngle + halfArc, 360.0f);

    // Mark all existing contacts stale
    for (RadarContact& c : mContacts)
        c.updated = false;

    // For each tracked object...
    for (const TrackedObject& tracked : mTrackedObjects)
    {
        SceneObject* obj = tracked.object;
        if (!obj || !obj->isProperlyAdded())
            continue;

        // 1) Compute its radar‐space 2D position
        Point2F radarPos = worldToRadar(obj->getPosition(), center, xf);
        F32     dist = Point2F(radarPos.x, radarPos.y).len();

        if (dist < 0.01f)
            continue;  // too close to origin

        bool shouldUpdate = false;

        if (mRadarMode == Sweep)
        {
            // 2) Angle in [0,360)
            F32 localAngle = mFmod(mRadToDeg(mAtan2(radarPos.y, radarPos.x)) + 360.0f, 360.0f);

            // 3) Wrap‐safe in‐arc test
            if (sweepStart < sweepEnd)
                shouldUpdate = (localAngle >= sweepStart && localAngle <= sweepEnd);
            else
                shouldUpdate = (localAngle >= sweepStart || localAngle <= sweepEnd);
        }
        else // Pulse mode unchanged
        {
            const F32 margin = 8.0f;
            shouldUpdate = (dist >= (mPulseRing.radius - margin)
                && dist <= (mPulseRing.radius + margin));
        }

        if (!shouldUpdate)
            continue;

        // 4) Update or create the contact entry
        Point3F worldPos = obj->getPosition();
        RadarContact* found = nullptr;
        for (RadarContact& c : mContacts)
        {
            if ((c.worldPos - worldPos).lenSquared() < 0.01f)
            {
                found = &c;
                break;
            }
        }

        if (found)
        {
            found->worldPos = worldPos;
            found->alpha = 1.0f;
            found->updated = true;
        }
        else
        {
            RadarContact newC;
            newC.worldPos = worldPos;
            newC.alpha = 1.0f;
            newC.updated = true;
            mContacts.push_back(newC);
        }
    }
}

Point2F GuiRadarTrackerCtrl::worldToRadar(const Point3F& pos, const Point3F& center, const MatrixF& xf)
{
    Point3F rel = pos - center;
    Point3F fwd;
    xf.getColumn(1, &fwd); // Y axis is forward
    F32 yaw = mAtan2(fwd.x, fwd.y);
    MatrixF rot;
    rot.set(EulerF(0, 0, -yaw));
    rot.mulP(rel);
    return Point2F(rel.x / mZoom, rel.y / mZoom);
}

void GuiRadarTrackerCtrl::addTrackedObject(SceneObject* obj)
{
    if (!obj)
        return;
    for (auto& t : mTrackedObjects)
        if (t.object == obj)
            return;
    mTrackedObjects.push_back({ obj });
}

void GuiRadarTrackerCtrl::removeTrackedObject(SceneObject* obj)
{
    for (S32 i = 0; i < mTrackedObjects.size(); ++i)
    {
        if (mTrackedObjects[i].object == obj)
        {
            mTrackedObjects.erase(i);
            break;
        }
    }
}

void GuiRadarTrackerCtrl::clearTrackedObjects()
{
    mTrackedObjects.clear();
}

void GuiRadarTrackerCtrl::setRadarMode(U32 mode)
{
    mRadarMode = (mode == 0) ? Pulse : Sweep;
}

void GuiRadarTrackerCtrl::setZoom(F32 zoom)
{
    mZoom = zoom;
}

void GuiRadarTrackerCtrl::setScanSpeed(F32 speed)
{
    mScanSpeed = speed;
}

void GuiRadarTrackerCtrl::setPulseSpeed(F32 speed)
{
    mPulseSpeed = speed;
}

void GuiRadarTrackerCtrl::setSweepTrail(F32 arc, U32 steps)
{
    mSweepTrailArc = arc;
    mSweepTrailSteps = steps;
}

void GuiRadarTrackerCtrl::setCenterObject(SceneObject* obj)
{
    mCenterObject = obj;
}

F32 GuiRadarTrackerCtrl::getPlayerYawDegrees() const
{
    if (!mCenterObject)
        return 0.f;
    MatrixF xf = mCenterObject->getTransform();
    VectorF fwd;
    xf.getColumn(1, &fwd); // forward is column 1
    return mRadToDeg(mAtan2(fwd.x, fwd.y));
}

// ------------------ Console Binds ------------------

DefineEngineMethod(GuiRadarTrackerCtrl, addTrackedObject, void, (SceneObject* obj), , "Add object to radar")
{
    object->addTrackedObject(obj);
}

DefineEngineMethod(GuiRadarTrackerCtrl, removeTrackedObject, void, (SceneObject* obj), , "Remove object from radar")
{
    object->removeTrackedObject(obj);
}

DefineEngineMethod(GuiRadarTrackerCtrl, clearTrackedObjects, void, (), , "Clear tracked objects")
{
    object->clearTrackedObjects();
}

DefineEngineMethod(GuiRadarTrackerCtrl, setCenterObject, void, (SceneObject* obj), , "Set center object (typically player)")
{
    object->setCenterObject(obj);
}

DefineEngineMethod(GuiRadarTrackerCtrl, setRadarMode, void, (U32 mode), , "Set radar mode: 0 = pulse, 1 = sweep")
{
    object->setRadarMode(mode);
}

DefineEngineMethod(GuiRadarTrackerCtrl, setZoom, void, (F32 zoom), , "Set zoom scale (meters per pixel)")
{
    object->setZoom(zoom);
}

DefineEngineMethod(GuiRadarTrackerCtrl, setScanSpeed, void, (F32 speed), , "Set sweep speed in degrees/sec")
{
    object->setScanSpeed(speed);
}

DefineEngineMethod(GuiRadarTrackerCtrl, setPulseSpeed, void, (F32 speed), , "Set pulse ring expansion speed (meters/sec)")
{
    object->setPulseSpeed(speed);
}

DefineEngineMethod(GuiRadarTrackerCtrl, setSweepTrail, void, (F32 arc, U32 steps), , "Set sweep arc and number of trail steps")
{
    object->setSweepTrail(arc, steps);
}

void GuiRadarTrackerCtrl::processTick()
{
}

void GuiRadarTrackerCtrl::interpolateTick(F32 delta)
{
}

void GuiRadarTrackerCtrl::advanceTime(F32 timeDelta)
{
}
