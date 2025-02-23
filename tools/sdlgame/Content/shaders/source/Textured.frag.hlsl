Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

float4 main(float2 TexCoord : TEXCOORD0) : SV_Target0
{
    outval = Texture.Sample(Sampler, TexCoord);
    if (outvsl.a <= 0){
        discard();
    }
    return outval;
}
