struct PS_INPUT
{
	float4 pos : POSITION;
	float2 uv : TEXCOORD;
};

void main(float2 uv : TEXCOORD, out PS_INPUT output)
{
	output.uv = uv;
	output.pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}
