cbuffer CBPackUnpackTest : register( b0 )
{
    float4 gInput;
}

RWTexture2D<float2> gPackedRG32 : register( u0 );
RWTexture2D<float4> gUnpackedRGBA32 : register( u1 );

[numthreads(1, 1, 1)]
void CSMain( uint3 dispatchThreadID : SV_DispatchThreadID )
{
    uint packedX = ( f32tof16( gInput.x ) & 0xFFFFu ) | ( ( f32tof16( gInput.y ) & 0xFFFFu ) << 16 );
    uint packedY = ( f32tof16( gInput.z ) & 0xFFFFu ) | ( ( f32tof16( gInput.w ) & 0xFFFFu ) << 16 );

    float2 packedAsFloat = float2( asfloat( packedX ), asfloat( packedY ) );
    gPackedRG32[uint2(0, 0)] = packedAsFloat;

    uint2 unpackBits = uint2( asuint( packedAsFloat.x ), asuint( packedAsFloat.y ) );
    float4 unpacked = float4(
        f16tof32( unpackBits.x & 0xFFFFu ),
        f16tof32( ( unpackBits.x >> 16 ) & 0xFFFFu ),
        f16tof32( unpackBits.y & 0xFFFFu ),
        f16tof32( ( unpackBits.y >> 16 ) & 0xFFFFu )
    );

    gUnpackedRGBA32[uint2(0, 0)] = unpacked;
}
