#include "spaceSceneLayer.h"
#include "gfx/primBuilder.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"
#include "console/sim.h"
#include "materials/shaderData.h"
#include "math/mathUtils.h"

IMPLEMENT_CO_NETOBJECT_V1(SpaceSceneLayer);

//-----------------------------------------------------------------------------
// Vertex format for our planet mesh
//-----------------------------------------------------------------------------
GFXImplementVertexFormat(GFXPlanetVert)
{
    addElement("TEXCOORD", GFXDeclType_Float2, 0);  // TEXCOORD0
    addElement("NORMAL", GFXDeclType_Float3, 1);  // NORMAL   → TEXCOORD1
    addElement("BINORMAL", GFXDeclType_Float3, 2);  // BINORMAL → TEXCOORD2
    addElement("TANGENT", GFXDeclType_Float3, 3);  // TANGENT  → TEXCOORD3
}

GFXImplementVertexFormat(GFXStarVert)
{
    addElement("POSITION", GFXDeclType_Float3, 0);   // POSITION → clip-space
    addElement("TEXCOORD", GFXDeclType_Float2, 0);   // TEXCOORD0
}

static const U32 smVertStride = 64;
static const U32 smStrideMinusOne = smVertStride - 1;
static const U32 smVertCount = smVertStride * smVertStride;
static const U32 smTriangleCount = smStrideMinusOne * smStrideMinusOne * 2;

SpaceSceneLayer::SpaceSceneLayer()
{
    mTypeMask |= EnvironmentObjectType | StaticObjectType;
    mNetFlags.set(Ghostable | ScopeAlways);

    mStarsShader = nullptr;
    mStarsConsts = nullptr;
    mModelViewProjSC = mEyePosWorldSC = mHazeIntensitySC = nullptr;
    mNebulaIntensitySC = mTimeSC = mStarColorSC = mRenderPlanetSC = nullptr;

    mPlanetShader = nullptr;
    mPlanetConsts = nullptr;
    mPlanetMVPSC = mPlanetEyeSC = mPlanetColorSC = mPlanetRenderSC = nullptr;

    mHazeIntensity = 0.0005f;
    mNebulaIntensity = 1.0f;
    mPlanetSize = 5000.0f;
    mPlanetDistance = 10000.0f;
    mRenderPlanet = true;
    mRenderStars = true;
    mStarColor.set(1, 1, 1);
    mPlanetColor.set(0.5f, 0.6f, 1.0f);
}

void SpaceSceneLayer::initPersistFields()
{
    addGroup("SpaceSceneLayer");
    addField("hazeIntensity", TypeF32, Offset(mHazeIntensity, SpaceSceneLayer));
    addField("nebulaIntensity", TypeF32, Offset(mNebulaIntensity, SpaceSceneLayer));
    addField("planetSize", TypeF32, Offset(mPlanetSize, SpaceSceneLayer));
    addField("planetDistance", TypeF32, Offset(mPlanetDistance, SpaceSceneLayer));
    addField("renderPlanet", TypeBool, Offset(mRenderPlanet, SpaceSceneLayer));
    addField("renderStars", TypeBool, Offset(mRenderStars, SpaceSceneLayer));
    addField("starColor", TypeColorF, Offset(mStarColor, SpaceSceneLayer));
    addField("planetColor", TypeColorF, Offset(mPlanetColor, SpaceSceneLayer));
    endGroup("SpaceSceneLayer");

    Parent::initPersistFields();
}

bool SpaceSceneLayer::onAdd()
{
    if (!Parent::onAdd())
        return false;

    Con::printf("[SpaceSceneLayer] onAdd()");
    setGlobalBounds();
    resetWorldBox();

    addToScene();

    if (isClientObject())
    {
        mObjBox = Box3F(Point3F(-1e6f), Point3F(1e6f));
        Con::printf("[SpaceSceneLayer] Initializing quad, planet geometry, nebula, and shaders...");

        _initQuad();
        _initPlanetGeometry();
        _initNebulaGeometry();
        _initShader();
    }

    return true;
}


void SpaceSceneLayer::onRemove()
{
    removeFromScene();
    Parent::onRemove();
}

void SpaceSceneLayer::prepRenderImage(SceneRenderState* state)
{
    Con::printf("[SpaceSceneLayer] prepRenderImage()");
    if (!mStarsShader) {
        Con::warnf("[SpaceSceneLayer] No stars shader bound. Aborting render.");
        return;
    }

    if (mRenderStars)
    {
        Con::printf("[SpaceSceneLayer] Scheduling renderStars and renderNebula()");
        ObjectRenderInst* ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
        ri->renderDelegate.bind(this, &SpaceSceneLayer::renderStars);
        ri->type = RenderPassManager::RIT_Sky;
        ri->defaultKey = 0;
        state->getRenderPass()->addInst(ri);

        ObjectRenderInst* ri2 = state->getRenderPass()->allocInst<ObjectRenderInst>();
        ri2->renderDelegate.bind(this, &SpaceSceneLayer::renderNebula);
        ri2->type = RenderPassManager::RIT_Sky;
        ri2->defaultKey = 1;
        state->getRenderPass()->addInst(ri2);
    }

    if (mRenderPlanet && mPlanetShader)
    {
        Con::printf("[SpaceSceneLayer] Scheduling renderPlanet()");
        ObjectRenderInst* ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
        ri->renderDelegate.bind(this, &SpaceSceneLayer::renderPlanet);
        ri->type = RenderPassManager::RIT_Sky;
        ri->defaultKey = 2;
        state->getRenderPass()->addInst(ri);
    }
}

U32 SpaceSceneLayer::packUpdate(NetConnection* conn, U32 mask, BitStream* stream)
{
    return Parent::packUpdate(conn, mask, stream);
}

void SpaceSceneLayer::unpackUpdate(NetConnection* conn, BitStream* stream)
{
    Parent::unpackUpdate(conn, stream);
}

//-----------------------------------------------------------------------------
// Draw the sky (stars + nebula) via full-screen quad
//-----------------------------------------------------------------------------
void SpaceSceneLayer::renderStars(ObjectRenderInst*, SceneRenderState* state, BaseMatInstance*)
{
    Con::printf("[SpaceSceneLayer] renderStars()");
    GFXTransformSaver saver;
    const Point3F camPos = state->getCameraPosition();
    Con::printf("Camera Pos: %g %g %g", camPos.x, camPos.y, camPos.z);

    MatrixF world(true);
    world.setPosition(camPos);
    GFX->multWorld(world);

    MatrixF view = GFX->getViewMatrix();
    MatrixF proj = GFX->getProjectionMatrix();
    MatrixF worldMatrix = GFX->getWorldMatrix();
    MatrixF mvp = proj * view * worldMatrix;

    F32* data = mvp;
    Con::printf("MVP Matrix:");
    for (U32 i = 0; i < 4; ++i)
    {
        Con::printf("  %g %g %g %g", data[i * 4 + 0], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
    }

    if (state->isReflectPass())
        GFX->setProjectionMatrix(state->getSceneManager()->getNonClipProjection());

    GFX->setShader(mStarsShader);
    GFX->setShaderConstBuffer(mStarsConsts);
    GFX->setStateBlock(mStateblock);

    mStarsConsts->setSafe(mModelViewProjSC, mvp);
    mStarsConsts->setSafe(mEyePosWorldSC, camPos);
    mStarsConsts->setSafe(mHazeIntensitySC, mHazeIntensity);
    mStarsConsts->setSafe(mNebulaIntensitySC, mNebulaIntensity);
    mStarsConsts->setSafe(mTimeSC, F32(Sim::getCurrentTime()) * 0.001f);
    mStarsConsts->setSafe(mStarColorSC, Point3F(mStarColor));
    mStarsConsts->setSafe(mRenderPlanetSC, 0.0f);

    Con::printf("[SpaceSceneLayer] Drawing fullscreen quad...");
    GFX->setVertexBuffer(mQuadVB);
    GFX->setPrimitiveBuffer(mQuadPB);
    GFX->drawIndexedPrimitive(GFXTriangleList, 0, 0, 4, 0, 2);
}

//-----------------------------------------------------------------------------
// Draw the planet sphere
//-----------------------------------------------------------------------------
void SpaceSceneLayer::renderPlanet(ObjectRenderInst*, SceneRenderState* state, BaseMatInstance*)
{
    if (!mPlanetVB.isValid() || !mPlanetPB.isValid())
        return;

    GFXTransformSaver saver;
    const Point3F camPos = state->getCameraPosition();
    MatrixF world(true); world.setPosition(camPos);
    GFX->multWorld(world);

    if (state->isReflectPass())
        GFX->setProjectionMatrix(state->getSceneManager()->getNonClipProjection());

    // bind planet shader
    GFX->setShader(mPlanetShader);
    GFX->setShaderConstBuffer(mPlanetConsts);
    GFX->setStateBlock(mPlanetState);

    // upload uniforms
    MatrixF mvp = GFX->getProjectionMatrix() * GFX->getViewMatrix() * GFX->getWorldMatrix();

    if (!mPlanetMVPSC || !mPlanetConsts)
    {
        Con::errorf("SpaceSceneLayer::_initShader - Failed to get handle for $modelViewProj");
        return;
    }

    mPlanetConsts->setSafe(mPlanetMVPSC, mvp);
    mPlanetConsts->setSafe(mPlanetEyeSC, camPos);
    mPlanetConsts->setSafe(mPlanetColorSC, Point3F(mPlanetColor.red,
        mPlanetColor.green,
        mPlanetColor.blue));
    mPlanetConsts->setSafe(mPlanetRenderSC, mRenderPlanet ? 1.0f : 0.0f);

    // draw sphere
    GFX->setVertexBuffer(mPlanetVB);
    GFX->setPrimitiveBuffer(mPlanetPB);
    GFX->drawIndexedPrimitive(GFXTriangleList,
        0, 0,
        mPlanetVB->mNumVerts,
        0,
        mPlanetPB->mPrimitiveCount);
}

void SpaceSceneLayer::renderNebula(ObjectRenderInst*, SceneRenderState* state, BaseMatInstance*)
{
    if (!mNebulaVB.isValid() || !mNebulaPB.isValid())
        return;

    GFXTransformSaver saver;
    const Point3F camPos = state->getCameraPosition();
    MatrixF world(true);
    world.setPosition(camPos); // nebula dome follows camera
    GFX->multWorld(world);

    if (state->isReflectPass())
        GFX->setProjectionMatrix(state->getSceneManager()->getNonClipProjection());

    GFX->setShader(mStarsShader);
    GFX->setShaderConstBuffer(mStarsConsts);
    GFX->setStateBlock(mStateblock);

    // set shared uniforms (same as renderStars)
    MatrixF mvp = GFX->getProjectionMatrix() * GFX->getViewMatrix() * GFX->getWorldMatrix();
    mStarsConsts->setSafe(mModelViewProjSC, mvp);
    mStarsConsts->setSafe(mEyePosWorldSC, camPos);
    mStarsConsts->setSafe(mHazeIntensitySC, mHazeIntensity);
    mStarsConsts->setSafe(mNebulaIntensitySC, mNebulaIntensity);
    mStarsConsts->setSafe(mTimeSC, F32(Sim::getCurrentTime()) * 0.001f);
    mStarsConsts->setSafe(mStarColorSC, Point3F(mStarColor));
    mStarsConsts->setSafe(mRenderPlanetSC, mRenderPlanet ? 1.0f : 0.0f);

    // draw haze dome
    GFX->setVertexBuffer(mNebulaVB);
    GFX->setPrimitiveBuffer(mNebulaPB);
    GFX->drawIndexedPrimitive(GFXTriangleList, 0, 0, mNebulaVB->mNumVerts, 0, mNebulaPB->mPrimitiveCount);
}


//-----------------------------------------------------------------------------
// Load both sky + planet shaders and create their state blocks
//-----------------------------------------------------------------------------
void SpaceSceneLayer::_initShader()
{
    // — SKY SHADER —
    ShaderData* sd;

    if (!Sim::findObject("SpaceSceneShader", sd)) // <-- Corrected name
    {
        Con::errorf("Missing ShaderData \"SpaceSceneShader\"!");
        return;
    }

    if (!sd)
    {
        Con::errorf("Missing ShaderData \"SpaceSceneShader\"!");
        return;
    }

    mStarsShader = sd->getShader();

    if (!mStarsShader)
    {
        Con::errorf("SpaceSceneLayer::_initShader - getShader() returned NULL for SpaceSceneShader");
        return;
    }

    // Dump out what constants the shader actually has:
    const auto& descs = mStarsShader->getShaderConstDesc();
    Con::printf("StarsShader constants (%d):", descs.size());
    for (U32 i = 0; i < descs.size(); ++i)
        Con::printf("  [%d] name=%s bindPoint=%d size=%d",
            i, descs[i].name.c_str(), descs[i].bindPoint, descs[i].size);

    mStarsConsts = mStarsShader->allocConstBuffer();
    mModelViewProjSC = mStarsShader->getShaderConstHandle("$modelViewProj");
    mEyePosWorldSC = mStarsShader->getShaderConstHandle("$eyePosWorld");
    mHazeIntensitySC = mStarsShader->getShaderConstHandle("$hazeIntensity");
    mNebulaIntensitySC = mStarsShader->getShaderConstHandle("$nebulaIntensity");
    mTimeSC = mStarsShader->getShaderConstHandle("$time");
    mStarColorSC = mStarsShader->getShaderConstHandle("$starColor");
    mRenderPlanetSC = mStarsShader->getShaderConstHandle("$renderPlanet");

    GFXStateBlockDesc sb;
    sb.setCullMode(GFXCullNone);
    sb.setBlend(true);
    sb.setZReadWrite(true, false);
    sb.zFunc = GFXCmpLessEqual;
    sb.samplersDefined = false;

    mStateblock = GFX->createStateBlock(sb);

    // — PLANET SHADER —
    ShaderData* psd;

    if (!Sim::findObject("SpacePlanetShader", psd))
    {
        Con::errorf("Missing ShaderData \"SpacePlanetShader\"!");
        return;
    }

    mPlanetShader = psd->getShader();

    if (!mPlanetShader)
    {
        Con::errorf("SpaceSceneLayer::_initShader - getShader() returned NULL for SpacePlanetShader");
        return;
    }

    mPlanetConsts = mPlanetShader->allocConstBuffer();

    if (!mPlanetConsts)
    {
        Con::errorf("SpaceSceneLayer::_initShader - allocConstBuffer() returned NULL for SpacePlanetShader");
        return;
    }

    // Dump out what constants the shader actually has:
    const auto& descs2 = mPlanetShader->getShaderConstDesc();
    Con::printf("SpacePlanetShader constants (%d):", descs2.size());
    for (U32 i = 0; i < descs2.size(); ++i)
        Con::printf("  [%d] name=%s bindPoint=%d size=%d",
            i, descs2[i].name.c_str(), descs2[i].bindPoint, descs2[i].size);

    // Finally grab our handles (they should be valid now):
    mPlanetMVPSC = mPlanetShader->getShaderConstHandle("$modelViewProj");
    mPlanetEyeSC = mPlanetShader->getShaderConstHandle("$eyePosWorld");
    mPlanetColorSC = mPlanetShader->getShaderConstHandle("$planetColor");
    mPlanetRenderSC = mPlanetShader->getShaderConstHandle("$renderPlanet");

    // create planet stateblock…
    GFXStateBlockDesc pd;
    pd.setCullMode(GFXCullNone);
    pd.setZReadWrite(true, true);
    pd.zFunc = GFXCmpLessEqual;
    pd.setBlend(false);
    mPlanetState = GFX->createStateBlock(pd);
}

//-----------------------------------------------------------------------------
// Build a full‐screen quad (clip‐space) for sky pass.
//-----------------------------------------------------------------------------
void SpaceSceneLayer::_initQuad()
{
    static const GFXStarVert quadVerts[] = {
        { {-1,-1,1}, {0,0} },
        { {-1, 1,1}, {0,1} },
        { { 1,-1,1}, {1,0} },
        { { 1, 1,1}, {1,1} }
    };

    static const U16 quadIdx[] = { 0,1,2, 2,1,3 };

    mQuadVB.set(GFX, 4, GFXBufferTypeStatic);
    {
        auto v = mQuadVB.lock();
        dMemcpy(v, quadVerts, sizeof(quadVerts));
        mQuadVB.unlock();
    }
    mQuadPB.set(GFX, 6, 2, GFXBufferTypeStatic);
    {
        U16* i = nullptr;
        mQuadPB.lock(&i);
        dMemcpy(i, quadIdx, sizeof(quadIdx));
        mQuadPB.unlock();
    }
}

//-----------------------------------------------------------------------------
// Build a skinned sphere for the planet pass.
//-----------------------------------------------------------------------------
void SpaceSceneLayer::_initPlanetGeometry()
{
    const U32 slices = 32, stacks = 24;
    Vector<Point3F> verts; verts.reserve((slices + 1) * (stacks + 1));

    // positions
    for (U32 y = 0; y <= stacks; ++y)
    {
        F32 phi = M_PI * F32(y) / F32(stacks);
        for (U32 x = 0; x <= slices; ++x)
        {
            F32 theta = M_2PI * F32(x) / F32(slices);
            Point3F p(
                mSin(phi) * mCos(theta),
                mCos(phi),
                mSin(phi) * mSin(theta)
            );
            verts.push_back(p * mPlanetSize + Point3F(0, 0, mPlanetDistance));
        }
    }

    // indices
    Vector<U16> idx; idx.reserve(stacks * slices * 6);
    for (U32 y = 0; y < stacks; ++y)
        for (U32 x = 0; x < slices; ++x)
        {
            U16 a = y * (slices + 1) + x;
            U16 b = a + (slices + 1);
            U16 c = a + 1, d = b + 1;
            idx.push_back(a); idx.push_back(b); idx.push_back(c);
            idx.push_back(b); idx.push_back(d); idx.push_back(c);
        }

    // fill VB
    mPlanetVB.set(GFX, verts.size(), GFXBufferTypeStatic);
    {
        auto vptr = mPlanetVB.lock();
        // first write positions
        for (U32 i = 0; i < verts.size(); ++i)
            vptr[i].point = verts[i];

        // compute normals/tangents/binormals & UV
        for (U32 y = 0; y < stacks; ++y)
            for (U32 x = 0; x < slices; ++x)
            {
                U32 i0 = y * (slices + 1) + x;
                U32 i1 = i0 + 1;
                U32 i2 = i0 + (slices + 1);
                auto& P = verts[i0];
                auto& Pr = verts[i1];
                auto& Pf = verts[i2];
                Point3F f = Pf - P; f.normalize();
                Point3F r = Pr - P; r.normalize();
                Point3F n = mCross(f, r); n.normalize();
                F32 phi = M_PI * F32(y) / F32(stacks);
                F32 theta = M_2PI * F32(x) / F32(slices);
                Point2F uv(theta * 0.5f / M_PI + 0.5f, phi / M_PI);
                for (U32 ii : {i0, i1, i2})
                {
                    vptr[ii].normal = n;
                    vptr[ii].binormal = f;
                    vptr[ii].tangent = r;
                    vptr[ii].texCoord = uv;
                }
            }

        mPlanetVB.unlock();
    }

    // fill PB
    mPlanetPB.set(GFX, idx.size(), idx.size() / 3, GFXBufferTypeStatic);
    {
        U16* ip = nullptr;
        mPlanetPB.lock(&ip);
        dMemcpy(ip, idx.address(), idx.size() * sizeof(U16));
        mPlanetPB.unlock();
    }
}

void SpaceSceneLayer::_initNebulaGeometry()
{
    Point3F vertScale(16000.0f, 16000.0f, mPlanetSize); // adjust scale to match sky
    F32 zOffset = -(mCos(mSqrt(1.0f)) + 0.01f);

    // Vertex Buffer
    mNebulaVB.set(GFX, smVertCount, GFXBufferTypeStatic);
    GFXPlanetVert* pVert = mNebulaVB.lock();
    if (!pVert)
        return;

    for (U32 y = 0; y < smVertStride; y++)
    {
        F32 v = ((F32)y / (F32)smStrideMinusOne - 0.5f) * 2.0f;

        for (U32 x = 0; x < smVertStride; x++)
        {
            F32 u = ((F32)x / (F32)smStrideMinusOne - 0.5f) * 2.0f;

            F32 sx = u;
            F32 sy = v;
            F32 sz = mCos(mSqrt(sx * sx + sy * sy)) + zOffset;
            pVert->point.set(sx, sy, sz);
            pVert->point *= vertScale;

            // The vertex to our right
            Point3F rpnt;
            F32 ru = ((F32)(x + 1) / (F32)smStrideMinusOne - 0.5f) * 2.0f;
            F32 rv = v;
            rpnt.set(ru, rv, mCos(mSqrt(ru * ru + rv * rv)) + zOffset);
            rpnt *= vertScale;

            // The vertex to our front
            Point3F fpnt;
            F32 fu = u;
            F32 fv = ((F32)(y + 1) / (F32)smStrideMinusOne - 0.5f) * 2.0f;
            fpnt.set(fu, fv, mCos(mSqrt(fu * fu + fv * fv)) + zOffset);
            fpnt *= vertScale;

            Point3F fvec = fpnt - pVert->point;
            fvec.normalize();
            Point3F rvec = rpnt - pVert->point;
            rvec.normalize();

            pVert->normal = mCross(fvec, rvec);
            pVert->normal.normalize();
            pVert->binormal = fvec;
            pVert->tangent = rvec;
            pVert->texCoord.set(u, v);

            ++pVert;
        }
    }
    mNebulaVB.unlock();

    // Primitive Buffer
    mNebulaPB.set(GFX, smTriangleCount * 3, smTriangleCount, GFXBufferTypeStatic);
    U16* pIdx = nullptr;
    mNebulaPB.lock(&pIdx);

    U32 curIdx = 0;
    for (U32 y = 0; y < smStrideMinusOne; y++)
    {
        for (U32 x = 0; x < smStrideMinusOne; x++)
        {
            U32 offset = x + y * smVertStride;

            pIdx[curIdx++] = offset;
            pIdx[curIdx++] = offset + 1;
            pIdx[curIdx++] = offset + smVertStride + 1;

            pIdx[curIdx++] = offset;
            pIdx[curIdx++] = offset + smVertStride + 1;
            pIdx[curIdx++] = offset + smVertStride;
        }
    }

    mNebulaPB.unlock();
}
