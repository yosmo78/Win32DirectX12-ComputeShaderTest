//cs_5_0 way
cbuffer globalCB : register(b0)
{
    uint4 dwOffsetsAndStrides0;
};

struct ModelOutData
{
	uint4 dwData;
};
//#define INDEX_BLOCK_SIZE 512
#define INDEX_BLOCK_SIZE 1

ByteAddressBuffer verticesAndIndices : register( t0 );
RWStructuredBuffer<ModelOutData> Out : register( u0 );

[RootSignature("RootFlags( 0 ), RootConstants( num32BitConstants=4, b0, space = 0, visibility=SHADER_VISIBILITY_ALL ), SRV(t0, space=0, visibility=SHADER_VISIBILITY_ALL), UAV(u0, space=0, visibility=SHADER_VISIBILITY_ALL)")]
[numthreads(INDEX_BLOCK_SIZE, 1, 1)]
void main( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex )
{
//TODO https://stackoverflow.com/questions/72249103/how-to-index-a-byteaddressbuffer-in-hlsl
	Out[0].dwData = 2 * dwOffsetsAndStrides0;
}