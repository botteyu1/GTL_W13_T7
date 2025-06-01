#include "DXDBufferManager.h"

#include <codecvt>
#include <locale>

void FDXDBufferManager::Initialize(ID3D11Device* InDXDevice, ID3D11DeviceContext* InDXDeviceContext)
{
    DXDevice = InDXDevice;
    DXDeviceContext = InDXDeviceContext;
    CreateQuadBuffer();
}

void FDXDBufferManager::ReleaseBuffers()
{
    for (auto& Pair : VertexBufferPool)
    {
        if (Pair.Value.VertexBuffer)
        {
            Pair.Value.VertexBuffer->Release();
            Pair.Value.VertexBuffer = nullptr;
        }
    }
    VertexBufferPool.Empty();

    for (auto& Pair : IndexBufferPool)
    {
        if (Pair.Value.IndexBuffer)
        {
            Pair.Value.IndexBuffer->Release();
            Pair.Value.IndexBuffer = nullptr;
        }
    }
    IndexBufferPool.Empty();
}

void FDXDBufferManager::ReleaseConstantBuffer()
{
    for (auto& Pair : ConstantBufferPool)
    {
        if (Pair.Value)
        {
            Pair.Value->Release();
            Pair.Value = nullptr;
        }
    }
    ConstantBufferPool.Empty();
}

void FDXDBufferManager::ReleaseStructuredBuffer()
{
    for (auto& Pair : StructuredBufferSRVPool)
    {
        if (Pair.Value)
        {
            Pair.Value->Release();
            Pair.Value = nullptr;
        }
    }
    StructuredBufferSRVPool.Empty();

    for (auto& Pair : StructuredBufferPool)
    {
        if (Pair.Value)
        {
            Pair.Value->Release();
            Pair.Value = nullptr;
        }
    }
    StructuredBufferPool.Empty();
}

HRESULT FDXDBufferManager::UpdateOrCreateDynamicVertexBuffer(
    const FString& KeyName, const void* pVertexData, uint32_t Stride, uint32_t NumVertices, FVertexInfo& OutVertexInfo
)
{
    if (pVertexData == nullptr && NumVertices > 0)
    {

    }

    uint32_t RequiredBufferSize = Stride * NumVertices;
    bool bNeedsNewBuffer = true;

    if (!KeyName.IsEmpty() && VertexBufferPool.Contains(KeyName))
    {
        OutVertexInfo = VertexBufferPool[KeyName];
        if (OutVertexInfo.VertexBuffer && OutVertexInfo.Stride * OutVertexInfo.NumVertices >= RequiredBufferSize)
        {
            // 기존 버퍼가 존재하고 크기가 충분하면 재사용 (데이터만 업데이트)
            bNeedsNewBuffer = false;
        }
        else
        {
            // 크기가 부족하거나 버퍼가 유효하지 않으면 기존 것 해제
            if (OutVertexInfo.VertexBuffer)
            {
                OutVertexInfo.VertexBuffer->Release();
                OutVertexInfo.VertexBuffer = nullptr;
            }
            VertexBufferPool.Remove(KeyName); // 풀에서 제거
        }
    }

    if (bNeedsNewBuffer)
    {
        if (NumVertices == 0) // 0개의 정점으로 버퍼를 만들 필요는 없음
        {
            OutVertexInfo.VertexBuffer = nullptr;
            OutVertexInfo.NumVertices = 0;
            OutVertexInfo.Stride = 0;
            if (!KeyName.IsEmpty()) VertexBufferPool.Remove(KeyName); // 키가 있었다면 제거
            return S_OK; // 또는 오류
        }

        D3D11_BUFFER_DESC BufferDesc = {};
        BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        BufferDesc.ByteWidth = RequiredBufferSize;
        BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // Map을 위해 필요

        // 동적 버퍼 생성 시 초기 데이터는 보통 제공하지 않거나,
        // pVertexData가 있다면 이를 사용할 수 있지만, Map/Unmap이 일반적입니다.
        // 여기서는 초기 데이터 없이 생성하고, 아래에서 Map/Unmap으로 채웁니다.
        HRESULT hr = DXDevice->CreateBuffer(&BufferDesc, nullptr, &OutVertexInfo.VertexBuffer);
        if (FAILED(hr))
        {
            OutVertexInfo.VertexBuffer = nullptr; // 실패 시 nullptr로 설정
            return hr;
        }

        OutVertexInfo.NumVertices = NumVertices;
        OutVertexInfo.Stride = Stride;
        if (!KeyName.IsEmpty())
        {
            VertexBufferPool.Add(KeyName, OutVertexInfo);
        }
    }

    // 버퍼에 데이터 업데이트 (생성되었거나 재사용될 때 모두)
    if (OutVertexInfo.VertexBuffer && pVertexData && NumVertices > 0)
    {
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        HRESULT hr = DXDeviceContext->Map(OutVertexInfo.VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
        if (SUCCEEDED(hr))
        {
            memcpy(MappedResource.pData, pVertexData, RequiredBufferSize);
            DXDeviceContext->Unmap(OutVertexInfo.VertexBuffer, 0);
        }
        else
        {
            return hr; // Map 실패 시 오류 반환
        }
    }
    // OutVertexInfo는 이미 최신 상태 (새로 생성되었거나 기존 것을 가져옴)
    return S_OK;
}

void FDXDBufferManager::BindConstantBuffers(const TArray<FString>& Keys, UINT StartSlot, EShaderStage Stage) const
{
    const int Count = Keys.Num();
    TArray<ID3D11Buffer*> Buffers;
    Buffers.Reserve(Count);
    for (const FString& Key : Keys)
    {
        ID3D11Buffer* Buffer = GetConstantBuffer(Key);
        Buffers.Add(Buffer);
    }

    if (Stage == EShaderStage::Vertex)
    {
        DXDeviceContext->VSSetConstantBuffers(StartSlot, Count, Buffers.GetData());
    }
    else if (Stage == EShaderStage::Pixel)
    {
        DXDeviceContext->PSSetConstantBuffers(StartSlot, Count, Buffers.GetData());
    }
    else if (Stage == EShaderStage::Compute)
    {
        DXDeviceContext->CSSetConstantBuffers(StartSlot, Count, Buffers.GetData());
    }
    else if (Stage == EShaderStage::Geometry)
    {
        DXDeviceContext->GSSetConstantBuffers(StartSlot, Count, Buffers.GetData());
    }
}   

void FDXDBufferManager::BindConstantBuffer(const FString& Key, UINT StartSlot, EShaderStage Stage) const
{
    ID3D11Buffer* Buffer = GetConstantBuffer(Key);
    if (Stage == EShaderStage::Vertex)
    {
        DXDeviceContext->VSSetConstantBuffers(StartSlot, 1, &Buffer);
    }
    else if (Stage == EShaderStage::Pixel)
    {
        DXDeviceContext->PSSetConstantBuffers(StartSlot, 1, &Buffer);
    }
    else if (Stage == EShaderStage::Compute)
    {
        DXDeviceContext->CSSetConstantBuffers(StartSlot, 1, &Buffer);
    }
    else if (Stage == EShaderStage::Geometry)
    {
        DXDeviceContext->GSSetConstantBuffers(StartSlot, 1, &Buffer);
    }
}

void FDXDBufferManager::BindStructuredBufferSRV(const FString& Key, UINT StartSlot, EShaderStage Stage) const
{
    ID3D11ShaderResourceView* SRV = GetStructuredBufferSRV(Key);
    if (Stage == EShaderStage::Vertex)
    {
        DXDeviceContext->VSSetShaderResources(StartSlot, 1, &SRV);
    }
    else if (Stage == EShaderStage::Pixel)
    {
        DXDeviceContext->PSSetShaderResources(StartSlot, 1, &SRV);
    }
    else if (Stage == EShaderStage::Compute)
    {
        DXDeviceContext->CSSetShaderResources(StartSlot, 1, &SRV);
    }
    else if (Stage == EShaderStage::Geometry)
    {
        DXDeviceContext->GSSetShaderResources(StartSlot, 1, &SRV);
    }
}

FVertexInfo FDXDBufferManager::GetVertexBuffer(const FString& InName) const
{
    if (VertexBufferPool.Contains(InName))
    {
        return VertexBufferPool[InName];
    }
    return FVertexInfo();
}

FIndexInfo FDXDBufferManager::GetIndexBuffer(const FString& InName) const
{
    if (IndexBufferPool.Contains(InName))
    {
        return IndexBufferPool[InName];
    }
    return FIndexInfo();
}

FVertexInfo FDXDBufferManager::GetTextVertexBuffer(const FWString& InName) const
{
    if (TextAtlasVertexBufferPool.Contains(InName))
    {
        return TextAtlasVertexBufferPool[InName];
    }
    return FVertexInfo();
}

FIndexInfo FDXDBufferManager::GetTextIndexBuffer(const FWString& InName) const
{
    if (TextAtlasIndexBufferPool.Contains(InName))
    {
        return TextAtlasIndexBufferPool[InName];
    }
    return FIndexInfo();
}

ID3D11Buffer* FDXDBufferManager::GetConstantBuffer(const FString& InName) const
{
    if (ConstantBufferPool.Contains(InName))
    {
        return ConstantBufferPool[InName];
    }
    return nullptr;
}

ID3D11Buffer* FDXDBufferManager::GetStructuredBuffer(const FString& InName) const
{
    if (StructuredBufferPool.Contains(InName))
    {
        return StructuredBufferPool[InName];
    }
    return nullptr;
}

ID3D11ShaderResourceView* FDXDBufferManager::GetStructuredBufferSRV(const FString& InName) const
{
    if (StructuredBufferSRVPool.Contains(InName))
    {
        return StructuredBufferSRVPool[InName];
    }
    return nullptr;
}

void FDXDBufferManager::CreateQuadBuffer()
{
    const TArray<QuadVertex> Vertices =
    {
        { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f} },
        { { 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f} },
        { { 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f} },
        { {-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },
    };

    FVertexInfo VertexInfo;
    CreateVertexBuffer(TEXT("QuadBuffer"), Vertices, VertexInfo);

    const TArray<short> Indices =
    {
        0, 1, 2,
        0, 2, 3
    };

    FIndexInfo IndexInfo;
    CreateIndexBuffer(TEXT("QuadBuffer"), Indices, IndexInfo);
}

void FDXDBufferManager::GetQuadBuffer(FVertexInfo& OutVertexInfo, FIndexInfo& OutIndexInfo)
{
    OutVertexInfo = GetVertexBuffer(TEXT("QuadBuffer"));
    OutIndexInfo = GetIndexBuffer(TEXT("QuadBuffer"));
}

void FDXDBufferManager::GetTextBuffer(const FWString& Text, FVertexInfo& OutVertexInfo, FIndexInfo& OutIndexInfo)
{
    OutVertexInfo = GetTextVertexBuffer(Text);
    OutIndexInfo = GetTextIndexBuffer(Text);
}


HRESULT FDXDBufferManager::CreateUnicodeTextBuffer(const FWString& Text, FBufferInfo& OutBufferInfo,
    float BitmapWidth, float BitmapHeight, float ColCount, float RowCount)
{
    if (TextAtlasBufferPool.Contains(Text))
    {
        OutBufferInfo = TextAtlasBufferPool[Text];
        return S_OK;
    }

    TArray<FVertexTexture> Vertices;

    // 각 글자에 대한 기본 쿼드 크기 (폭과 높이)
    constexpr float QuadWidth = 2.0f;
    [[maybe_unused]] constexpr float QuadHeight = 2.0f;

    // 전체 텍스트의 너비
    const float TotalTextWidth = QuadWidth * Text.size();
    // 텍스트의 중앙으로 정렬하기 위한 오프셋
    const float CenterOffset = TotalTextWidth / 2.0f;

    const float CellWidth = BitmapWidth / ColCount;                       // 컬럼별 셀 폭
    const float CellHeight = BitmapHeight / RowCount; // 행별 셀 높이

    const float TexelUOffset = CellWidth / BitmapWidth;
    const float TexelVOffset = CellHeight / BitmapHeight;

    for (int Idx = 0; Idx < Text.size(); Idx++)
    {
        // 각 글자에 대해 기본적인 사각형 좌표 설정 (원점은 -1.0f부터 시작)
        FVertexTexture LeftUp = { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f };
        FVertexTexture RightUp = { 1.0f,  1.0f, 0.0f, 1.0f, 0.0f };
        FVertexTexture LeftDown = { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f };
        FVertexTexture RightDown = { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f };

        // UV 좌표 관련 보정 (nTexel 오프셋 적용)
        RightUp.u *= TexelUOffset;
        LeftDown.v *= TexelVOffset;
        RightDown.u *= TexelUOffset;
        RightDown.v *= TexelVOffset;

        // 각 글자의 x 좌표에 대해 오프셋 적용 (중앙 정렬을 위해 centerOffset 만큼 빼줌)
        const float XOffset = QuadWidth * Idx - CenterOffset;
        LeftUp.x += XOffset;
        RightUp.x += XOffset;
        LeftDown.x += XOffset;
        RightDown.x += XOffset;

        FVector2D UVOffset;
        SetStartUV(Text[Idx], UVOffset);

        LeftUp.u += (TexelUOffset * UVOffset.X);
        LeftUp.v += (TexelVOffset * UVOffset.Y);
        RightUp.u += (TexelUOffset * UVOffset.X);
        RightUp.v += (TexelVOffset * UVOffset.Y);
        LeftDown.u += (TexelUOffset * UVOffset.X);
        LeftDown.v += (TexelVOffset * UVOffset.Y);
        RightDown.u += (TexelUOffset * UVOffset.X);
        RightDown.v += (TexelVOffset * UVOffset.Y);

        // 각 글자의 쿼드를 두 개의 삼각형으로 생성
        Vertices.Add(LeftUp);
        Vertices.Add(RightUp);
        Vertices.Add(LeftDown);
        Vertices.Add(RightUp);
        Vertices.Add(RightDown);
        Vertices.Add(LeftDown);
    }

    CreateVertexBuffer(Text, Vertices, OutBufferInfo.VertexInfo);
    TextAtlasBufferPool[Text] = OutBufferInfo;

    return S_OK;
}

void FDXDBufferManager::SetStartUV(wchar_t Hangul, FVector2D& UVOffset)
{
    //대문자만 받는중
    int StartU = 0;
    int StartV = 0;
    int Offset = -1;

    if (Hangul == L' ') {
        UVOffset = FVector2D(0, 0);  // Space는 특별히 UV 좌표를 (0,0)으로 설정
        Offset = 0;
        return;
    }
    else if (Hangul >= L'A' && Hangul <= L'Z') {

        StartU = 11;
        StartV = 0;
        Offset = Hangul - L'A'; // 대문자 위치
    }
    else if (Hangul >= L'a' && Hangul <= L'z') {
        StartU = 37;
        StartV = 0;
        Offset = (Hangul - L'a'); // 소문자는 대문자 다음 위치
    }
    else if (Hangul >= L'0' && Hangul <= L'9') {
        StartU = 1;
        StartV = 0;
        Offset = (Hangul - L'0'); // 숫자는 소문자 다음 위치
    }
    else if (Hangul >= L'가' && Hangul <= L'힣')
    {
        StartU = 63;
        StartV = 0;
        Offset = Hangul - L'가'; // 대문자 위치
    }

    if (Offset == -1)
    {
        UE_LOG(ELogLevel::Warning, "Text Error");
    }

    const int OffsetV = (Offset + StartU) / 106;
    const int OffsetU = (Offset + StartU) % 106;

    UVOffset = FVector2D(OffsetU, StartV + OffsetV);

}
