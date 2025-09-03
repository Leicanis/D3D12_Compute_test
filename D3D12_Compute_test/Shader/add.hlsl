StructuredBuffer<uint> InputBuffer : register(t0);
RWStructuredBuffer<uint> OutputBuffer : register(u0);

[numthreads(1, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    OutputBuffer[0] = InputBuffer[0] + InputBuffer[1];
}
