#pragma once

#include "RenderPassBase.h"
#include "Define.h" // PxVec3, PxU32 등 PhysX 타입 사용을 위해 (또는 physx/PxPhysicsAPI.h 직접 포함)
#include <PxPhysicsAPI.h> // PhysX API 직접 포함

class FDXDBufferManager;
class FGraphicsDevice;
class FDXDShaderManager;
class FEditorViewportClient;

// PhysX 디버그 렌더링을 위한 정점 구조체
struct FPhysicsDebugVertex
{
    FVector Position;
    FLinearColor Color; 
};

class FPhysicsDebugRenderPass : public FRenderPassBase
{
public:
    FPhysicsDebugRenderPass();
    virtual ~FPhysicsDebugRenderPass() override = default;

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    virtual void PrepareRenderArr() override; 
    virtual void ClearRenderArr() override;   
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

private:
    void CreateShadersAndInputLayout();
    void DrawDebugData(physx::PxScene* PhysicsScene, const std::shared_ptr<FEditorViewportClient>& Viewport);

    // 동적 버텍스 버퍼 (매 프레임 업데이트)
    // 또는, 더 큰 버퍼를 미리 할당하고 Map/Unmap으로 부분 업데이트
    // 여기서는 간단하게 매번 생성하거나, 작은 크기의 버퍼를 재활용하는 방식을 가정
    // ID3D11Buffer* DynamicVertexBuffer = nullptr;
    // UINT MaxDebugVertices = 1024 * 10; // 예시: 최대 디버그 정점 수
};
