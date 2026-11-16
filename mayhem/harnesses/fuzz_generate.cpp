// Fuzzes the generation API: a node (a single generator, a small fractal, or one of the Node
// Editor's demo trees) at a fuzzer-chosen SIMD level, driven through every Gen* entry point with
// fuzzer-chosen offsets, step sizes, seed and counts, into buffers of exactly the documented size.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "exercise.h"

static const char* kMountainTerrain = "E@BBZEG@BD8JFgIECArXIzwECiQIw/UoPwkuAAE@BJDQAH@BC@AIEAJBw@ABZEED0KV78YZmZmPwQDmpkZPwsAAIA/HAMAAHBCBA==";
static const char* kCellularCaves = "FgIcCS4AAQ@BklCQs@BlRBDNzMw9G@AIMAgAw@ADgC@BCiQIzczMPgkJ@BPkIQH4XrPhjNzEw/DBIkCM3MzD4JCQ@ADBCCAE@BQzczMvhg@B/JAL/BAAL7FE4PgQKFwkNCQg@CQQQDuB4FPwt7FC4/BAOPwnU8DA==";

static FastSIMD::FeatureSet PickFeatureSet( unsigned sel )
{
    static const FastSIMD::FeatureSet sets[] = {
        FastSIMD::FeatureSet::Max,
#if defined( __x86_64__ )
        FastSIMD::FeatureSet::SSE2,
        FastSIMD::FeatureSet::SSE41,
        FastSIMD::FeatureSet::AVX2,
        FastSIMD::FeatureSet::AVX512,
#endif
    };
    return sets[sel % ( sizeof( sets ) / sizeof( sets[0] ) )];
}

static FastNoise::SmartNode<> MakeNode( unsigned sel, FastSIMD::FeatureSet fs )
{
    switch( sel % 12 )
    {
    case 0: return FastNoise::New<FastNoise::Simplex>( fs );
    case 1: return FastNoise::New<FastNoise::SuperSimplex>( fs );
    case 2: return FastNoise::New<FastNoise::Perlin>( fs );
    case 3: return FastNoise::New<FastNoise::Value>( fs );
    case 4: return FastNoise::New<FastNoise::CellularValue>( fs );
    case 5: return FastNoise::New<FastNoise::CellularDistance>( fs );
    case 6: return FastNoise::New<FastNoise::Checkerboard>( fs );
    case 7: return FastNoise::New<FastNoise::White>( fs );
    case 8:
    {
        auto fractal = FastNoise::New<FastNoise::FractalFBm>( fs );
        auto source = FastNoise::New<FastNoise::Simplex>( fs );
        if( !fractal || !source )
        {
            return nullptr;
        }
        fractal->SetSource( source );
        return fractal;
    }
    case 9:
    {
        auto fractal = FastNoise::New<FastNoise::FractalRidged>( fs );
        auto source = FastNoise::New<FastNoise::Perlin>( fs );
        if( !fractal || !source )
        {
            return nullptr;
        }
        fractal->SetSource( source );
        return fractal;
    }
    case 10: return FastNoise::NewFromEncodedNodeTree( kMountainTerrain, fs );
    default: return FastNoise::NewFromEncodedNodeTree( kCellularCaves, fs );
    }
}

extern "C" int LLVMFuzzerTestOneInput( const uint8_t* data, size_t size )
{
    FuzzedDataProvider in( data, size );

    const FastSIMD::FeatureSet fs = PickFeatureSet( in.ConsumeIntegral<uint8_t>() );
    FastNoise::SmartNode<> node = MakeNode( in.ConsumeIntegral<uint8_t>(), fs );
    if( !node )
    {
        return -1;
    }

    const int api = in.ConsumeIntegralInRange( 0, 7 );
    const int seed = in.ConsumeIntegral<int>();
    const float xo = in.ConsumeFloatingPoint<float>(), yo = in.ConsumeFloatingPoint<float>();
    const float zo = in.ConsumeFloatingPoint<float>(), wo = in.ConsumeFloatingPoint<float>();
    const float xs = in.ConsumeFloatingPoint<float>(), ys = in.ConsumeFloatingPoint<float>();
    const float zs = in.ConsumeFloatingPoint<float>(), ws = in.ConsumeFloatingPoint<float>();

    switch( api )
    {
    case 0:
    {
        const int xc = in.ConsumeIntegralInRange( 1, 64 ), yc = in.ConsumeIntegralInRange( 1, 64 );
        std::vector<float> out( (size_t)xc * yc );
        node->GenUniformGrid2D( out.data(), xo, yo, xc, yc, xs, ys, seed );
        break;
    }
    case 1:
    {
        const int xc = in.ConsumeIntegralInRange( 1, 16 ), yc = in.ConsumeIntegralInRange( 1, 16 ), zc = in.ConsumeIntegralInRange( 1, 16 );
        std::vector<float> out( (size_t)xc * yc * zc );
        node->GenUniformGrid3D( out.data(), xo, yo, zo, xc, yc, zc, xs, ys, zs, seed );
        break;
    }
    case 2:
    {
        const int xc = in.ConsumeIntegralInRange( 1, 8 ), yc = in.ConsumeIntegralInRange( 1, 8 );
        const int zc = in.ConsumeIntegralInRange( 1, 8 ), wc = in.ConsumeIntegralInRange( 1, 8 );
        std::vector<float> out( (size_t)xc * yc * zc * wc );
        node->GenUniformGrid4D( out.data(), xo, yo, zo, wo, xc, yc, zc, wc, xs, ys, zs, ws, seed );
        break;
    }
    case 3:
    {
        const int xc = in.ConsumeIntegralInRange( 1, 32 ), yc = in.ConsumeIntegralInRange( 1, 32 );
        std::vector<float> out( (size_t)xc * yc );
        node->GenTileable2D( out.data(), xc, yc, xs, ys, seed );
        break;
    }
    case 4:
    case 5:
    case 6:
    {
        const int count = in.ConsumeIntegralInRange( 1, 256 );
        std::vector<float> out( count ), xp( count ), yp( count ), zp( count ), wp( count );
        for( int i = 0; i < count; i++ )
        {
            xp[i] = in.ConsumeFloatingPoint<float>();
            yp[i] = in.ConsumeFloatingPoint<float>();
            zp[i] = in.ConsumeFloatingPoint<float>();
            wp[i] = in.ConsumeFloatingPoint<float>();
        }
        if( api == 4 )
            node->GenPositionArray2D( out.data(), count, xp.data(), yp.data(), xo, yo, seed );
        else if( api == 5 )
            node->GenPositionArray3D( out.data(), count, xp.data(), yp.data(), zp.data(), xo, yo, zo, seed );
        else
            node->GenPositionArray4D( out.data(), count, xp.data(), yp.data(), zp.data(), wp.data(), xo, yo, zo, wo, seed );
        break;
    }
    default:
    {
        volatile float v = node->GenSingle4D( xo, yo, zo, wo, seed );
        v = node->GenSingle3D( xo, yo, zo, seed );
        v = node->GenSingle2D( xo, yo, seed );
        (void)v;
        break;
    }
    }
    return 0;
}
