// Known-answer probe for the oracle. Computes values from FastNoise2's public API on the lowest
// SIMD path (so the result does not depend on the CPU) and checks them against kat/expected.txt.
// Prints one "KAT PASS <name>" / "KAT FAIL <name> ..." line per case and "KAT DONE <n>" at the end.
// `kat --print` emits the current values in expected.txt format instead.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <FastNoise/FastNoise.h>
#include <FastNoise/Metadata.h>

#include "FastNoise/Base64.h"

using FastNoise::SmartNode;

#if defined( __x86_64__ )
static const FastSIMD::FeatureSet kSet = FastSIMD::FeatureSet::SSE2; // lowest compiled set, runs on every x86-64 CPU
#else
static const FastSIMD::FeatureSet kSet = FastSIMD::FeatureSet::Max;
#endif

static std::vector<std::pair<std::string, double>> gValues;
static std::vector<std::pair<std::string, bool>> gChecks;

static void Value( const std::string& name, double v ) { gValues.emplace_back( name, v ); }
static void Check( const std::string& name, bool ok ) { gChecks.emplace_back( name, ok ); }

template<typename T>
static void Single( const char* name )
{
    SmartNode<T> g = FastNoise::New<T>( kSet );
    if( !g ) { Check( std::string( name ) + ".create", false ); return; }
    static const float pts[][3] = { { 0.5f, 0.5f, 0.5f }, { 12.25f, -3.75f, 8.5f }, { -100.1f, 55.5f, 0.3f } };
    int i = 0;
    for( auto& p : pts )
    {
        Value( std::string( name ) + ".2d." + std::to_string( i ), g->GenSingle2D( p[0], p[1], 1337 ) );
        Value( std::string( name ) + ".3d." + std::to_string( i ), g->GenSingle3D( p[0], p[1], p[2], 1337 ) );
        i++;
    }
    Value( std::string( name ) + ".seed2", g->GenSingle2D( 12.25f, -3.75f, 42 ) );
}

static void Tree( const char* name, const char* encoded )
{
    SmartNode<> g = FastNoise::NewFromEncodedNodeTree( encoded, kSet );
    if( !g ) { Check( std::string( name ) + ".decode", false ); return; }
    float grid[16 * 16];
    FastNoise::OutputMinMax mm = g->GenUniformGrid2D( grid, 3.0f, -5.0f, 16, 16, 0.37f, 0.37f, 1337 );
    double sum = 0, wsum = 0;
    for( int i = 0; i < 256; i++ ) { sum += grid[i]; wsum += grid[i] * ( i % 7 + 1 ); }
    Value( std::string( name ) + ".sum", sum );
    Value( std::string( name ) + ".wsum", wsum );
    Value( std::string( name ) + ".min", mm.min );
    Value( std::string( name ) + ".max", mm.max );
    // Batch and single-sample paths must agree.
    Check( std::string( name ) + ".grid_vs_single", std::fabs( grid[17] - g->GenSingle2D( 3.0f + 0.37f, -5.0f + 0.37f, 1337 ) ) < 1e-3f );
}

int main( int argc, char** argv )
{
    const bool print = argc > 1 && !strcmp( argv[1], "--print" );

    Single<FastNoise::Simplex>( "simplex" );
    Single<FastNoise::SuperSimplex>( "supersimplex" );
    Single<FastNoise::Perlin>( "perlin" );
    Single<FastNoise::Value>( "value" );
    Single<FastNoise::CellularValue>( "cellvalue" );
    Single<FastNoise::CellularDistance>( "celldist" );
    Single<FastNoise::Checkerboard>( "checker" );
    Single<FastNoise::White>( "white" );

    Tree( "mountain", "E@BBZEG@BD8JFgIECArXIzwECiQIw/UoPwkuAAE@BJDQAH@BC@AIEAJBw@ABZEED0KV78YZmZmPwQDmpkZPwsAAIA/HAMAAHBCBA==" );
    Tree( "cellular_caves", "FgIcCS4AAQ@BklCQs@BlRBDNzMw9G@AIMAgAw@ADgC@BCiQIzczMPgkJ@BPkIQH4XrPhjNzEw/DBIkCM3MzD4JCQ@ADBCCAE@BQzczMvhg@B/JAL/BAAL7FE4PgQKFwkNCQg@CQQQDuB4FPwt7FC4/BAOPwnU8DA==" );

    // Hand-built tree: fBm over Perlin.
    {
        auto fractal = FastNoise::New<FastNoise::FractalFBm>( kSet );
        auto source = FastNoise::New<FastNoise::Perlin>( kSet );
        Check( "fbm_perlin.create", fractal && source );
        if( fractal && source )
        {
            fractal->SetSource( source );
            fractal->SetOctaveCount( 4 );
            Value( "fbm_perlin.2d", fractal->GenSingle2D( 7.5f, -2.25f, 99 ) );
            Value( "fbm_perlin.3d", fractal->GenSingle3D( 7.5f, -2.25f, 1.5f, 99 ) );
        }
    }

    // Library Base64: known vector and round trip.
    {
        const std::vector<uint8_t> bytes = { 0x0d, 0x09, 0x06, 0x0c };
        Check( "base64.encode", FastNoise::Base64::Encode( bytes ) == "DQkGDA==" );
        Check( "base64.decode", FastNoise::Base64::Decode( "DQkGDA==" ) == bytes );
        std::vector<uint8_t> all;
        for( int i = 0; i < 300; i++ ) all.push_back( (uint8_t)( i * 7 + 3 ) );
        Check( "base64.roundtrip", FastNoise::Base64::Decode( FastNoise::Base64::Encode( all ).c_str() ) == all );
    }

    // Documented example: "DQkGDA==" is a valid tree; garbage is rejected.
    Check( "decode.valid", (bool)FastNoise::NewFromEncodedNodeTree( "DQkGDA==", kSet ) );
    Check( "decode.garbage", !FastNoise::NewFromEncodedNodeTree( "this is not a node tree!", kSet ) );
    Check( "decode.empty", !FastNoise::NewFromEncodedNodeTree( "", kSet ) );

    // Metadata registry: node names and the editor-side round trip.
    {
        const auto& all = FastNoise::Metadata::GetAll();
        Value( "metadata.count", (double)all.size() );
        std::vector<std::unique_ptr<FastNoise::NodeData>> nodes;
        FastNoise::NodeData* root = FastNoise::Metadata::DeserialiseNodeData( "DQkGDA==", nodes );
        Check( "nodedata.deserialise", root != nullptr );
        if( root )
        {
            Check( "nodedata.reserialise", FastNoise::Metadata::SerialiseNodeData( root, true ) == "DQkGDA==" );
            Check( "nodedata.name", FastNoise::Metadata::FormatMetadataNodeName( root->metadata ) == "Fractal FBm" );
        }
    }

    if( print )
    {
        for( auto& v : gValues ) printf( "%s %.6f\n", v.first.c_str(), v.second );
        return 0;
    }

    std::map<std::string, double> expected;
    FILE* f = fopen( argc > 1 ? argv[1] : "mayhem/kat/expected.txt", "r" );
    if( !f ) { printf( "KAT FAIL expected.txt missing\n" ); return 1; }
    char name[256];
    double v;
    while( fscanf( f, "%255s %lf", name, &v ) == 2 ) expected[name] = v;
    fclose( f );

    int n = 0;
    for( auto& val : gValues )
    {
        auto it = expected.find( val.first );
        const double tol = 1e-3 * ( 1.0 + std::fabs( val.second ) );
        if( it != expected.end() && std::fabs( it->second - val.second ) <= tol )
            printf( "KAT PASS %s\n", val.first.c_str() );
        else
            printf( "KAT FAIL %s got %.6f expected %.6f\n", val.first.c_str(), val.second, it == expected.end() ? NAN : it->second );
        n++;
    }
    for( auto& c : gChecks )
    {
        printf( "KAT %s %s\n", c.second ? "PASS" : "FAIL", c.first.c_str() );
        n++;
    }
    printf( "KAT DONE %d\n", n );
    return 0;
}
