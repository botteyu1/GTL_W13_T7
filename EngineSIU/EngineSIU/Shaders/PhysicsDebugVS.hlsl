#include "ShaderRegisters.hlsl"

struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0; // 픽셀 셰이더로 색상 전달
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    // 언리얼 FVector는 이미 엔진 월드 좌표계이므로, 뷰-프로젝션 변환만 수행
    output.Position = mul(float4(input.Position, 1.0f), ViewMatrix);
    output.Position = mul(output.Position, ProjectionMatrix);
    output.Color = input.Color;
    return output;
}
