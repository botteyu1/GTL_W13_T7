#include "PhysicsDebugRenderPass.h"

#include "PhysicsManager.h"
#include "UnrealClient.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "UnrealEd/EditorViewportClient.h"
#include "Engine/EditorEngine.h" // GEngine 사용
#include "World/World.h"         // UWorld 사용


FPhysicsDebugRenderPass::FPhysicsDebugRenderPass()
{
    // 필요한 초기화
}

void FPhysicsDebugRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
    CreateShadersAndInputLayout();

}

void FPhysicsDebugRenderPass::CreateShadersAndInputLayout()
{
    D3D11_INPUT_ELEMENT_DESC DebugLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}, // FVector (12 bytes) 다음에 FLinearColor
    };

    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"PhysicsDebugVS", L"Shaders/PhysicsDebugVS.hlsl", "main", DebugLayoutDesc, ARRAYSIZE(DebugLayoutDesc));
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create PhysicsDebugVS shader!"));
    }

    hr = ShaderManager->AddPixelShader(L"PhysicsDebugPS", L"Shaders/PhysicsDebugPS.hlsl", "main");
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create PhysicsDebugPS shader!"));
    }
}


void FPhysicsDebugRenderPass::PrepareRenderArr()
{
    // 이 패스는 동적으로 PhysX 씬에서 데이터를 가져오므로, 미리 배열을 채울 필요가 없을 수 있습니다.
    // 만약 특정 조건의 PhysX 객체만 그리고 싶다면 여기서 필터링할 수 있습니다.
}

void FPhysicsDebugRenderPass::ClearRenderArr()
{
    // PrepareRenderArr에서 채운 데이터가 있다면 여기서 비웁니다.
}

void FPhysicsDebugRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (!GEngine || !GEngine->ActiveWorld)
        return;

    physx::PxScene* PhysicsScene = GEngine->PhysicsManager->GetScene(GEngine->ActiveWorld);
    
    if (!PhysicsScene)
        return;

    PrepareRender(Viewport);
    DrawDebugData(PhysicsScene, Viewport);
    CleanUpRender(Viewport);
}

void FPhysicsDebugRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->RSSetViewports(1, &Viewport->GetViewportResource()->GetD3DViewport());

    // 다른 패스와 동일한 렌더 타겟 및 뎁스 스텐실 사용
    constexpr EResourceType ResourceType = EResourceType::ERT_Scene;
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    const FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(ResourceType);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, nullptr);

    // 블렌드 상태 (디버그 라인은 보통 반투명하지 않음, 필요시 알파 블렌딩 설정)
    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    // 뎁스 테스트는 하되, 뎁스 쓰기는 상황에 따라 (다른 디버그 정보와 겹칠 때 어떻게 보일지)
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState_Default, 1); // 뎁스 테스트 및 쓰기 활성화

    // 셰이더 및 입력 레이아웃 설정
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"PhysicsDebugVS");
    ID3D11InputLayout* InputLayout = ShaderManager->GetInputLayoutByKey(L"PhysicsDebugVS");
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"PhysicsDebugPS");

    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    // 공통 상수 버퍼 바인딩 (예: 뷰-프로젝션 매트릭스)
    // BufferManager->BindConstantBuffer(TEXT("FViewProjectionConstantBuffer"), 0, EShaderStage::Vertex); // 엔진의 상수버퍼 이름으로
    // 디버그 렌더링용 색상 상수 버퍼 (선택 사항)
    // struct FDebugDrawConstants { DirectX::XMFLOAT4 DrawColor; };
    // BufferManager->BindConstantBuffer(TEXT("FDebugDrawConstants"), 1, EShaderStage::Pixel);
}

void FPhysicsDebugRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState_Default, 1);
}

void FPhysicsDebugRenderPass::DrawDebugData(physx::PxScene* PhysicsScene, const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // 어떤 항목을 시각화할지 설정 (매 프레임 또는 필요시 설정)
    PhysicsScene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, 1.0f);
    PhysicsScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
    //PhysicsScene->setVisualizationParameter(physx::PxVisualizationParameter::eJOINT_LIMITS, 1.0f);
    PhysicsScene->setVisualizationParameter(physx::PxVisualizationParameter::eCONTACT_POINT, 1.f); // 접촉점은 작게
    // ... 필요한 다른 파라미터들 ...

    const physx::PxRenderBuffer& rb = PhysicsScene->getRenderBuffer();

    // 임시 버텍스 데이터를 담을 TArray
    TArray<FPhysicsDebugVertex> LineVertices;
    TArray<FPhysicsDebugVertex> PointVertices; // 점은 다르게 처리하거나, 짧은 선으로 표현 가능
    TArray<FPhysicsDebugVertex> TriangleVertices;

    // 선 데이터 처리
    if (rb.getNbLines() > 0)
    {
        LineVertices.Reserve(rb.getNbLines() * 2);
        for (physx::PxU32 i = 0; i < rb.getNbLines(); i++)
        {
            const physx::PxDebugLine& line = rb.getLines()[i];
            LineVertices.Add({ FVector(line.pos0), FLinearColor(line.color0) });
            LineVertices.Add({ FVector(line.pos1), FLinearColor(line.color1) });
        }

        if (!LineVertices.IsEmpty())
        {
            FVertexInfo VertexInfo;
            HRESULT hr = BufferManager->UpdateOrCreateDynamicVertexBuffer(
                TEXT("PhysicsDebug_Lines"), // 고유한 키 이름
                LineVertices.GetData(),
                sizeof(FPhysicsDebugVertex),
                LineVertices.Num(),
                VertexInfo
            );
            
            if (SUCCEEDED(hr) && VertexInfo.VertexBuffer)
            {
                UINT stride = sizeof(FPhysicsDebugVertex);
                UINT offset = 0;
                Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &stride, &offset);
                Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                Graphics->DeviceContext->Draw(LineVertices.Num(), 0);
            }
        }
    }

    // 점 데이터 처리
    if (rb.getNbPoints() > 0)
    {
        PointVertices.Reserve(rb.getNbPoints());
        for (physx::PxU32 i = 0; i < rb.getNbPoints(); i++)
        {
            const physx::PxDebugPoint& point = rb.getPoints()[i];
            PointVertices.Add({ FVector(point.pos), FLinearColor(point.color) });
        }
        if (!PointVertices.IsEmpty())
        {
            FVertexInfo VertexInfo;
            HRESULT hr = BufferManager->UpdateOrCreateDynamicVertexBuffer(
                TEXT("PhysicsDebugPointVB"), // 고유한 키 이름
                PointVertices.GetData(),
                sizeof(FPhysicsDebugVertex),
                PointVertices.Num(),
                VertexInfo
            );
            if (VertexInfo.VertexBuffer)
            {
                UINT stride = sizeof(FPhysicsDebugVertex);
                UINT offset = 0;
                Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &stride, &offset);
                Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
                Graphics->DeviceContext->Draw(PointVertices.Num(), 0);
            }
        }
    }

    // 삼각형 데이터 처리
    if (rb.getNbTriangles() > 0)
    {
        TriangleVertices.Reserve(rb.getNbTriangles() * 3);
        for (physx::PxU32 i = 0; i < rb.getNbTriangles(); i++)
        {
            const physx::PxDebugTriangle& tri = rb.getTriangles()[i];
            TriangleVertices.Add({ FVector(tri.pos0), FLinearColor(tri.color0) });
            TriangleVertices.Add({ FVector(tri.pos1), FLinearColor(tri.color1) });
            TriangleVertices.Add({ FVector(tri.pos2), FLinearColor(tri.color2) });
        }
        if (!TriangleVertices.IsEmpty())
        {
            FVertexInfo VertexInfo;
            HRESULT hr = BufferManager->UpdateOrCreateDynamicVertexBuffer(
                TEXT("PhysicsDebugTriangleVB"), // 고유한 키 이름
                TriangleVertices.GetData(),
                sizeof(FPhysicsDebugVertex),
                TriangleVertices.Num(),
                VertexInfo
            );
            if (VertexInfo.VertexBuffer)
            {
                UINT stride = sizeof(FPhysicsDebugVertex);
                UINT offset = 0;
                Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &stride, &offset);
                Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                Graphics->DeviceContext->Draw(TriangleVertices.Num(), 0);
            }
        }
    }
}
