/*     Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include <cmath>
#include <array>

#include "SSAOSample.h"
#include "MapHelper.h"
#include "BasicMath.h"
#include "GraphicsUtilities.h"
#include "AntTweakBar.h"
#include "TextureUtilities.h"
#include "CommonlyUsedStates.h"
#include "ShaderMacroHelper.h"
#include "FileSystem.h"

namespace Diligent
{

#include "BasicStructures.fxh"

SampleBase* CreateSample()
{
    return new SSAOSample();
}


void SSAOSample::Initialize(IEngineFactory* pEngineFactory, IRenderDevice* pDevice, IDeviceContext** ppContexts, Uint32 NumDeferredCtx, ISwapChain* pSwapChain)
{
    SampleBase::Initialize(pEngineFactory, pDevice, ppContexts, NumDeferredCtx, pSwapChain);

    m_SSAO.reset(new SSAOPostProcess(m_pDevice, m_pImmediateContext, m_pSwapChain->GetDesc().ColorBufferFormat, m_pSwapChain->GetDesc().DepthBufferFormat));

    RefCntAutoPtr<ITexture> EnvironmentMap;
    {
        // Create uniform environment map
        TextureDesc EnvMapDesc;
        EnvMapDesc.Name        = "Irradiance cube map";
        EnvMapDesc.Type        = RESOURCE_DIM_TEX_CUBE;
        EnvMapDesc.Usage       = USAGE_STATIC;
        EnvMapDesc.BindFlags   = BIND_SHADER_RESOURCE;
        EnvMapDesc.Width       = 32;
        EnvMapDesc.Height      = 32;
        EnvMapDesc.Format      = TEX_FORMAT_RGBA32_FLOAT;
        EnvMapDesc.ArraySize   = 6;
        EnvMapDesc.MipLevels   = 1;
        std::vector<float> InitData(EnvMapDesc.Width * EnvMapDesc.Height * 4, 0.5f);
        std::array<TextureSubResData, 6> SubResData;
        for(auto& face : SubResData)
        {
            face.pData  = InitData.data();
            face.Stride = EnvMapDesc.Width * sizeof(float) * 4;
        }
        TextureData TexInitData{SubResData.data(), 6};
        m_pDevice->CreateTexture(EnvMapDesc, &TexInitData, &EnvironmentMap);
        m_EnvironmentMapSRV = EnvironmentMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }

    auto BackBufferFmt  = m_pSwapChain->GetDesc().ColorBufferFormat;
    auto DepthBufferFmt = m_pSwapChain->GetDesc().DepthBufferFormat;
    GLTF_PBR_Renderer::CreateInfo RendererCI;
    RendererCI.RTVFmt         = BackBufferFmt;
    RendererCI.DSVFmt         = DepthBufferFmt;
    RendererCI.AllowDebugView = false;
    RendererCI.UseIBL         = true;
    m_GLTFRenderer.reset(new GLTF_PBR_Renderer(m_pDevice, m_pImmediateContext, RendererCI));

    CreateUniformBuffer(m_pDevice, sizeof(CameraAttribs),       "Camera attribs buffer",         &m_CameraAttribsCB);
    CreateUniformBuffer(m_pDevice, sizeof(LightAttribs),        "Light attribs buffer",          &m_LightAttribsCB);
    StateTransitionDesc Barriers [] =
    {
        {m_CameraAttribsCB,        RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true},
        {m_LightAttribsCB,         RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_CONSTANT_BUFFER, true},
        {EnvironmentMap,           RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_SHADER_RESOURCE, true}
    };
    m_pImmediateContext->TransitionResourceStates(_countof(Barriers), Barriers);

    m_GLTFRenderer->PrecomputeCubemaps(m_pDevice, m_pImmediateContext, m_EnvironmentMapSRV);

    m_LightDirection  = normalize(float3(0.5f, -0.6f, -0.2f));

    // Create a tweak bar
    TwBar *bar = TwNewBar("Settings");
    int barSize[2] = {250 * m_UIScale, 600 * m_UIScale};
    TwSetParam(bar, NULL, "size", TW_PARAM_INT32, 2, barSize);

    TwAddVarRW(bar, "Light direction",    TW_TYPE_DIR3F,   &m_LightDirection,  "opened=true axisz=-z");
    TwAddVarRW(bar, "Light Color",        TW_TYPE_COLOR4F, &m_LightColor,      "group='Lighting' opened=false");
    TwAddVarRW(bar, "Light Intensity",    TW_TYPE_FLOAT,   &m_LightIntensity,  "group='Lighting' min=0.0 max=50.0 step=0.1");
    TwAddVarRW(bar, "Occlusion strength", TW_TYPE_FLOAT,   &m_RenderParams.OcclusionStrength, "group='Lighting' min=0.0 max=1.0 step=0.01");
    TwAddVarRW(bar, "Emission scale",     TW_TYPE_FLOAT,   &m_RenderParams.EmissionScale,     "group='Lighting' min=0.0 max=1.0 step=0.01");
    TwAddVarRW(bar, "IBL scale",          TW_TYPE_FLOAT,   &m_RenderParams.IBLScale,          "group='Lighting' min=0.0 max=1.0 step=0.01");
    TwAddVarRW(bar, "Average log lum",    TW_TYPE_FLOAT,   &m_RenderParams.AverageLogLum,     "group='Tone mapping' min=0.01 max=10.0 step=0.01");
    TwAddVarRW(bar, "Middle gray",        TW_TYPE_FLOAT,   &m_RenderParams.MiddleGray,        "group='Tone mapping' min=0.01 max=1.0 step=0.01");
    TwAddVarRW(bar, "White point",        TW_TYPE_FLOAT,   &m_RenderParams.WhitePoint,        "group='Tone mapping' min=0.1  max=20.0 step=0.1");
    TwAddVarRW(bar, "Enable SSAO",        TW_TYPE_BOOLCPP, &m_EnableSSAO,                     "");
    
    m_Model.reset(new GLTF::Model(m_pDevice, m_pImmediateContext, "sponza/Sponza.gltf"));
    m_GLTFRenderer->InitializeResourceBindings(*m_Model, m_CameraAttribsCB, m_LightAttribsCB);

    m_ModelTransform = float4x4::Translation(float3(0, -3.f, 0));

    m_Camera.SetPos(float3(-6, 0, -0.5f));
    m_Camera.SetRotation(PI_F/2.f, 0);
    m_Camera.SetRotationSpeed(0.005f);
    m_Camera.SetMoveSpeed(2.f);
    m_Camera.SetSpeedUpScales(5.f, 10.f);
}

SSAOSample::~SSAOSample()
{
}

void SSAOSample::WindowResize(Uint32 Width, Uint32 Height)
{
    RefCntAutoPtr<ITexture> pOffscreenRenderTarget;
    TextureDesc ColorDesc;
    ColorDesc.Type        = RESOURCE_DIM_TEX_2D;
    ColorDesc.Width       = m_pSwapChain->GetDesc().Width;
    ColorDesc.Height      = m_pSwapChain->GetDesc().Height;
    ColorDesc.MipLevels   = 1;
    ColorDesc.Format      = m_pSwapChain->GetDesc().ColorBufferFormat;
    ColorDesc.BindFlags   = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    ColorDesc.ClearValue.Format = ColorDesc.Format;
    ColorDesc.ClearValue.Color[0] = 0.032f;
    ColorDesc.ClearValue.Color[1] = 0.032f;
    ColorDesc.ClearValue.Color[2] = 0.032f;
    ColorDesc.ClearValue.Color[3] = 1.f;
    m_pDevice->CreateTexture(ColorDesc, nullptr, &pOffscreenRenderTarget);
    m_pColorRTV = pOffscreenRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    m_pColorSRV = pOffscreenRenderTarget->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    

    // Render target depth attachment
    RefCntAutoPtr<ITexture> pDepthBuffer;
    TextureDesc DepthDesc = ColorDesc;
    DepthDesc.Format = DepthBufferFormat;
    DepthDesc.ClearValue.Format = DepthDesc.Format;
    DepthDesc.ClearValue.DepthStencil.Depth = 1;
    DepthDesc.ClearValue.DepthStencil.Stencil = 0;
    DepthDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
    m_pDevice->CreateTexture(DepthDesc, nullptr, &pDepthBuffer);
    m_pDepthDSV = pDepthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
    m_pDepthSRV = pDepthBuffer->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    m_SSAO->OnWindowResize(m_pDevice, Width, Height);

    float NearPlane = 0.1f;
    float FarPlane = 100.f;
    float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    m_Camera.SetProjAttribs(NearPlane, FarPlane, AspectRatio, PI_F / 4.f, m_pDevice->GetDeviceCaps().IsGLDevice());
}

// Render a frame
void SSAOSample::Render()
{
    const float ClearColor[] = { 0.032f,  0.032f,  0.032f, 1.0f }; 
    m_pImmediateContext->ClearRenderTarget(nullptr, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (m_EnableSSAO)
    {
        ITextureView* pRTVs[] = {m_pColorRTV};
        m_pImmediateContext->SetRenderTargets(_countof(pRTVs), pRTVs, m_pDepthDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearRenderTarget(m_pColorRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearDepthStencil(m_pDepthDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    else
    {
        m_pImmediateContext->ClearDepthStencil(nullptr, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    const auto& CameraView  = m_Camera.GetViewMatrix();
    const auto& CameraWorld = m_Camera.GetWorldMatrix();
    float3 CameraWorldPos = float3::MakeVector(CameraWorld[3]);
    const auto& Proj = m_Camera.GetProjMatrix();
    auto CameraViewProj = CameraView * Proj;

    {
        MapHelper<CameraAttribs> CamAttribs(m_pImmediateContext, m_CameraAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
        CamAttribs->mProjT        = Proj.Transpose();
        CamAttribs->mViewProjT    = CameraViewProj.Transpose();
        CamAttribs->mViewProjInvT = CameraViewProj.Inverse().Transpose();
        CamAttribs->f4Position = float4(CameraWorldPos, 1);
    }

    {
        MapHelper<LightAttribs> lightAttribs(m_pImmediateContext, m_LightAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
        lightAttribs->f4Direction = m_LightDirection;
        lightAttribs->f4Intensity = m_LightColor * m_LightIntensity;
    }

    m_RenderParams.ModelTransform = m_ModelTransform;
    m_GLTFRenderer->Render(m_pImmediateContext, *m_Model, m_RenderParams);

    if (m_EnableSSAO)
    {
        SSAOPostProcess::FrameAttribs Attribs;
        Attribs.pCameraAttribsCB = m_CameraAttribsCB;
        Attribs.pDevice          = m_pDevice;
        Attribs.pDeviceContext   = m_pImmediateContext;
        Attribs.ptex2DSrcColorBufferSRV = m_pColorSRV;
        Attribs.ptex2DSrcDepthBufferSRV = m_pDepthSRV;
        Attribs.pDstRTV                 = nullptr; // Render to the default framebuffer
        m_SSAO->PerformPostProcessing(Attribs);
    }
}


void SSAOSample::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);
    m_Camera.Update(m_InputController, static_cast<float>(ElapsedTime));
}

}
