/*     Copyright 2019 Diligent Graphics LLC
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

#pragma once 

#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include "SampleBase.h"
#include "BasicMath.h"
#include "ThreadSignal.h"

namespace Diligent
{

class Tutorial09_Quads final : public SampleBase
{
public:
    ~Tutorial09_Quads()override;
    virtual void GetEngineInitializationAttribs(DeviceType          DevType, 
                                                EngineCreateInfo&   Attribs)override final;
    virtual void Initialize(IEngineFactory*  pEngineFactory,
                            IRenderDevice*   pDevice, 
                            IDeviceContext** ppContexts, 
                            Uint32           NumDeferredCtx, 
                            ISwapChain*      pSwapChain)override final;
    virtual void Render()override final;
    virtual void Update(double CurrTime, double ElapsedTime)override final;
    virtual const Char* GetSampleName()const override final{return "Tutorial09: Quads";}

private:
    void CreatePipelineStates(std::vector<StateTransitionDesc>& Barriers);
    void LoadTextures        (std::vector<StateTransitionDesc>& Barriers);
    void UpdateUI();

    void InitializeQuads();
    void CreateInstanceBuffer();
    void UpdateQuads(float elapsedTime);
    void StartWorkerThreads(size_t NumThreads);
    void StopWorkerThreads();
    template<bool UseBatch>
    void RenderSubset(IDeviceContext *pCtx, Uint32 Subset);

    static void WorkerThreadFunc(Tutorial09_Quads *pThis, Uint32 ThreadNum);

    ThreadingTools::Signal m_RenderSubsetSignal;
    ThreadingTools::Signal m_ExecuteCommandListsSignal;
    ThreadingTools::Signal m_GotoNextFrameSignal;
    std::mutex m_NumThreadsCompletedMtx;
    std::atomic_int m_NumThreadsCompleted;
    std::atomic_int m_NumThreadsReady;
    std::vector<std::thread> m_WorkerThreads;
    std::vector< RefCntAutoPtr<ICommandList> > m_CmdLists;

    static constexpr int NumStates = 5;
    RefCntAutoPtr<IPipelineState> m_pPSO[2][NumStates];
    RefCntAutoPtr<IBuffer> m_QuadAttribsCB;
    RefCntAutoPtr<IBuffer> m_BatchDataBuffer;

    static constexpr int NumTextures = 4;
    RefCntAutoPtr<IShaderResourceBinding> m_SRB[NumTextures];
    RefCntAutoPtr<IShaderResourceBinding> m_BatchSRB;
    RefCntAutoPtr<ITextureView> m_TextureSRV[NumTextures];
    RefCntAutoPtr<ITextureView> m_TexArraySRV;
    int m_NumQuads  = 1000;
    int m_BatchSize = 5;

    int m_MaxThreads       = 8;
    int m_NumWorkerThreads = 4;

    struct QuadData
    {
        float2 Pos;
        float2 MoveDir;
        float Size;
        float Angle;
        float RotSpeed;
        int TextureInd;
        int StateInd;
    };
    std::vector<QuadData> m_Quads;

    struct InstanceData
    {
        float4 QuadRotationAndScale;
        float2 QuadCenter;
        float TexArrInd;
    };
};

}
