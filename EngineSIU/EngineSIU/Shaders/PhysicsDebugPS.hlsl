struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    return input.Color; // 버텍스에서 전달된 색상 사용
}
