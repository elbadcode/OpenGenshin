texture2D t0 : register(t0);
sampler2D s0 : register(s0);

void main(float4 vpos : VPOS, float2 uv : TEXCOORD, out float4 col : COLOR)
{
	col = tex2D(s0, uv);
	col.a = 1.0; // Clear alpha channel
}
