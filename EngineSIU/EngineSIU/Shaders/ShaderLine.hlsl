
#include "ShaderRegisters.hlsl"

cbuffer GridParametersData : register(b1)
{
    float GridSpacing;
    int GridCount; // 총 grid 라인 수
    float2 Padding1;
    
    float3 GridOrigin; // Grid의 중심
    float Padding;
};

cbuffer PrimitiveCounts : register(b3)
{
    int BoundingBoxCount;
    int pad0;
    int ConeCount;
    int pad1;
    int DebugLineCount;
    int pad2;
    int OBBCount;
    int pad3;
};



struct FBoundingBoxData
{
    float3 bbMin;
    float padding0;
    float3 bbMax;
    float padding1;
};
struct FConeData
{
    float3 ConeApex; // 원뿔의 꼭짓점
    float ConeRadius; // 원뿔 밑면 반지름
    
    float3 ConeBaseCenter; // 원뿔 밑면 중심
    float ConeHeight; // 원뿔 높이 (Apex와 BaseCenter 간 차이)
    float4 Color;
    
    int ConeSegmentCount; // 원뿔 밑면 분할 수
    float pad[3];
};
struct FOrientedBoxCornerData
{
    float4 corners[8]; // 회전/이동 된 월드 공간상의 8꼭짓점
};
struct FDebugLineData
{
    float3 Start;
    float pad0;
    float3 End;
    float pad1;
    float4 Color;
};

StructuredBuffer<FBoundingBoxData> g_BoundingBoxes : register(t2);
StructuredBuffer<FConeData> g_ConeData : register(t3);
StructuredBuffer<FOrientedBoxCornerData> g_OrientedBoxes : register(t4);
StructuredBuffer<FDebugLineData> g_DebugLines : register(t5);
static const int BB_EdgeIndices[12][2] =
{
    { 0, 1 },
    { 1, 3 },
    { 3, 2 },
    { 2, 0 }, // 앞면
    { 4, 5 },
    { 5, 7 },
    { 7, 6 },
    { 6, 4 }, // 뒷면
    { 0, 4 },
    { 1, 5 },
    { 2, 6 },
    { 3, 7 } // 측면
};

struct VS_INPUT
{
    uint vertexID : SV_VertexID; // 0 또는 1: 각 라인의 시작과 끝
    uint instanceID : SV_InstanceID; // 인스턴스 ID로 grid, axis, bounding box를 구분
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 WorldPosition : POSITION;
    float4 Color : COLOR;
    uint instanceID : SV_InstanceID;
};

/////////////////////////////////////////////////////////////////////////
// Grid 위치 계산 함수
/////////////////////////////////////////////////////////////////////////
float3 ComputeGridPosition(uint instanceID, uint vertexID)
{
    int halfCount = GridCount / 2;
    float centerOffset = halfCount * 0.5; // grid 중심이 원점에 오도록

    float3 startPos;
    float3 endPos;
    
    if (instanceID < halfCount)
    {
        // 수직선: X 좌표 변화, Y는 -centerOffset ~ +centerOffset
        float x = GridOrigin.x + (instanceID - centerOffset) * GridSpacing;
        if (abs(x - GridOrigin.x) < 0.001)
        {
            startPos = float3(0, 0, 0);
            endPos = float3(0, (GridOrigin.y - centerOffset * GridSpacing), 0);
        }
        else
        {
            startPos = float3(x, GridOrigin.y - centerOffset * GridSpacing, GridOrigin.z);
            endPos = float3(x, GridOrigin.y + centerOffset * GridSpacing, GridOrigin.z);
        }
    }
    else
    {
        // 수평선: Y 좌표 변화, X는 -centerOffset ~ +centerOffset
        int idx = instanceID - halfCount;
        float y = GridOrigin.y + (idx - centerOffset) * GridSpacing;
        if (abs(y - GridOrigin.y) < 0.001)
        {
            startPos = float3(0, 0, 0);
            endPos = float3(-(GridOrigin.x + centerOffset * GridSpacing), 0, 0);
        }
        else
        {
            startPos = float3(GridOrigin.x - centerOffset * GridSpacing, y, GridOrigin.z);
            endPos = float3(GridOrigin.x + centerOffset * GridSpacing, y, GridOrigin.z);
        }

    }
    return (vertexID == 0) ? startPos : endPos;
}

/////////////////////////////////////////////////////////////////////////
// Axis 위치 계산 함수 (총 3개: X, Y, Z)
/////////////////////////////////////////////////////////////////////////
float3 ComputeAxisPosition(uint axisInstanceID, uint vertexID)
{
    float3 start = float3(0.0, 0.0, 0.0);
    float3 end;
    float zOffset = 0.f;
    if (axisInstanceID == 0)
    {
        // X 축: 빨간색
        end = float3(1000000.0, 0.0, zOffset);
    }
    else if (axisInstanceID == 1)
    {
        // Y 축: 초록색
        end = float3(0.0, 1000000.0, zOffset);
    }
    else if (axisInstanceID == 2)
    {
        // Z 축: 파란색
        end = float3(0.0, 0.0, 1000000.0 + zOffset);
    }
    else
    {
        end = start;
    }
    return (vertexID == 0) ? start : end;
}

/////////////////////////////////////////////////////////////////////////
// Bounding Box 위치 계산 함수
// bbInstanceID: StructuredBuffer에서 몇 번째 bounding box인지
// edgeIndex: 해당 bounding box의 12개 엣지 중 어느 것인지
/////////////////////////////////////////////////////////////////////////
float3 ComputeBoundingBoxPosition(uint bbInstanceID, uint edgeIndex, uint vertexID)
{
    FBoundingBoxData box = g_BoundingBoxes[bbInstanceID];
  
//    0: (bbMin.x, bbMin.y, bbMin.z)
//    1: (bbMax.x, bbMin.y, bbMin.z)
//    2: (bbMin.x, bbMax.y, bbMin.z)
//    3: (bbMax.x, bbMax.y, bbMin.z)
//    4: (bbMin.x, bbMin.y, bbMax.z)
//    5: (bbMax.x, bbMin.y, bbMax.z)
//    6: (bbMin.x, bbMax.y, bbMax.z)
//    7: (bbMax.x, bbMax.y, bbMax.z)
    int vertIndex = BB_EdgeIndices[edgeIndex][vertexID];
    float x = ((vertIndex & 1) == 0) ? box.bbMin.x : box.bbMax.x;
    float y = ((vertIndex & 2) == 0) ? box.bbMin.y : box.bbMax.y;
    float z = ((vertIndex & 4) == 0) ? box.bbMin.z : box.bbMax.z;
    return float3(x, y, z);
}

/////////////////////////////////////////////////////////////////////////
// Axis 위치 계산 함수 (총 3개: X, Y, Z)
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////
// Cone 계산 함수
/////////////////////////////////////////////////
// Helper: 계산된 각도에 따른 밑면 꼭짓점 위치 계산

float3 ComputeConePosition(uint globalInstanceID, uint vertexID)
{
    // 모든 cone이 동일한 세그먼트 수를 가짐
    int N = g_ConeData[0].ConeSegmentCount;
    
    uint coneIndex = globalInstanceID / (2 * N);
    uint lineIndex = globalInstanceID % (2 * N);
    
    // cone 데이터 읽기
    FConeData cone = g_ConeData[coneIndex];
    
    // cone의 축 계산
    float3 axis = normalize(cone.ConeApex - cone.ConeBaseCenter);
    
    // axis에 수직인 두 벡터(u, v)를 생성
    float3 arbitrary = abs(dot(axis, float3(0, 0, 1))) < 0.99 ? float3(0, 0, 1) : float3(0, 1, 0);
    float3 u = normalize(cross(axis, arbitrary));
    float3 v = cross(axis, u);
    
    if (lineIndex < (uint) N)
    {
        // 측면 선분: cone의 꼭짓점과 밑면의 한 점을 잇는다.
        float angle = lineIndex * 6.28318530718 / N;
        float3 baseVertex = cone.ConeBaseCenter + (cos(angle) * u + sin(angle) * v) * cone.ConeRadius;
        return (vertexID == 0) ? cone.ConeApex : baseVertex;
    }
    else
    {
        // 밑면 둘레 선분: 밑면상의 인접한 두 점을 잇는다.
        uint idx = lineIndex - N;
        float angle0 = idx * 6.28318530718 / N;
        float angle1 = ((idx + 1) % N) * 6.28318530718 / N;
        float3 v0 = cone.ConeBaseCenter + (cos(angle0) * u + sin(angle0) * v) * cone.ConeRadius;
        float3 v1 = cone.ConeBaseCenter + (cos(angle1) * u + sin(angle1) * v) * cone.ConeRadius;
        return (vertexID == 0) ? v0 : v1;
    }
}
/////////////////////////////////////////////////////////////////////////
// OBB
/////////////////////////////////////////////////////////////////////////
float3 ComputeOrientedBoxPosition(uint obIndex, uint edgeIndex, uint vertexID)
{
    FOrientedBoxCornerData ob = g_OrientedBoxes[obIndex];
    int cornerID = BB_EdgeIndices[edgeIndex][vertexID];
    return ob.corners[cornerID].xyz;
}

float3 ComputeDebugLinePosition(uint debugIndex, uint vertexID)
{
    FDebugLineData LineData;
    LineData = g_DebugLines[debugIndex];
    return (vertexID == 0) ? LineData.Start : LineData.End;
}
/////////////////////////////////////////////////////////////////////////
// 메인 버텍스 셰이더
/////////////////////////////////////////////////////////////////////////

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    float3 pos;
    float4 color;

    // Instance Counts
    uint gridLineCount = GridCount;
    uint axisCount = 3;
    uint aabbInstanceCount = 12 * BoundingBoxCount;
    int coneSegmentCount = g_ConeData[0].ConeSegmentCount;
    uint coneInstanceCount = ConeCount * 2 * coneSegmentCount;
    uint obbInstanceCount = 12 * OBBCount;
    uint debugLineCount = DebugLineCount;

    // Offsets
    uint coneStart = gridLineCount + axisCount + aabbInstanceCount;
    uint obbStart = coneStart + coneInstanceCount;
    uint debugStart = obbStart + obbInstanceCount;

    // Instance dispatch
    if (input.instanceID < gridLineCount)
    {
        // Grid
        pos = ComputeGridPosition(input.instanceID, input.vertexID);
        color = float4(0.1, 0.1, 0.1, 0.75);
    }
    else if (input.instanceID < gridLineCount + axisCount)
    {
        // Axis
        uint axisInstanceID = input.instanceID - gridLineCount;
        pos = ComputeAxisPosition(axisInstanceID, input.vertexID);

        if (axisInstanceID == 0)
            color = float4(1.0, 0.0, 0.0, 1.0); // X: Red
        else if (axisInstanceID == 1)
            color = float4(0.0, 1.0, 0.0, 1.0); // Y: Green
        else
            color = float4(0.0, 0.0, 1.0, 1.0); // Z: Blue
    }
    else if (input.instanceID < gridLineCount + axisCount + aabbInstanceCount)
    {
        // AABB
        uint index = input.instanceID - (gridLineCount + axisCount);
        uint bbInstanceID = index / 12;
        uint bbEdgeIndex = index % 12;

        pos = ComputeBoundingBoxPosition(bbInstanceID, bbEdgeIndex, input.vertexID);
        color = float4(1.0, 1.0, 0.0, 1.0); // Yellow
    }
    else if (input.instanceID < obbStart)
    {
        // Cone
        uint coneInstanceID = input.instanceID - coneStart;
        pos = ComputeConePosition(coneInstanceID, input.vertexID);
        uint coneIndex = coneInstanceID / (2 * coneSegmentCount);
        color = g_ConeData[coneIndex].Color;
    }
    else if (input.instanceID < debugStart)
    {
        // OBB
        uint obbLocalID = input.instanceID - obbStart;
        uint obbIndex = obbLocalID / 12;
        uint edgeIndex = obbLocalID % 12;

        pos = ComputeOrientedBoxPosition(obbIndex, edgeIndex, input.vertexID);
        color = float4(0.4, 1.0, 0.4, 1.0); // Light green
    }
    else
    {
        // DebugLine
        uint debugIndex = input.instanceID - debugStart;
        FDebugLineData LineData = g_DebugLines[debugIndex];
        pos = ComputeDebugLinePosition(debugIndex, input.vertexID);

        color = LineData.Color;
    }

    // Position transform
    output.Position = float4(pos, 1.f);
    output.Position = mul(output.Position, WorldMatrix);
    output.WorldPosition = output.Position;

    output.Position = mul(output.Position, ViewMatrix);
    output.Position = mul(output.Position, ProjectionMatrix);

    output.Color = color;
    output.instanceID = input.instanceID;
    return output;
}


float4 mainPS(PS_INPUT input) : SV_Target
{
    if (input.instanceID < GridCount || input.instanceID < GridCount + 3)
    {
        float Dist = length(input.WorldPosition.xyz - ViewWorldLocation);

        float MaxDist = 400 * 1.2f;
        float MinDist = MaxDist * 0.3f;

         // Fade out grid
        float Fade = saturate(1.f - (Dist - MinDist) / (MaxDist - MinDist));
        input.Color.a *= Fade * Fade * Fade;
    }
    return input.Color;
}
