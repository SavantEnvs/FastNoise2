// Fuzzes the editor-side tree model: Metadata::DeserialiseNodeData, then SerialiseNodeData with
// fix-up (which nulls dependency loops and invalid links), then loads the re-encoded string as a
// real node tree and generates from it.
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <FastNoise/Metadata.h>

#include "exercise.h"

extern "C" int LLVMFuzzerTestOneInput( const uint8_t* data, size_t size )
{
    const std::string encoded( reinterpret_cast<const char*>( data ), size );

    std::vector<std::unique_ptr<FastNoise::NodeData>> nodeData;
    FastNoise::NodeData* root = FastNoise::Metadata::DeserialiseNodeData( encoded.c_str(), nodeData );
    if( !root )
    {
        return -1;
    }

    const std::string reencoded = FastNoise::Metadata::SerialiseNodeData( root, true );
    if( reencoded.empty() )
    {
        return 0;
    }

    FastNoise::SmartNode<> node = FastNoise::NewFromEncodedNodeTree( reencoded.c_str() );
    if( node )
    {
        ExerciseNode( node );
    }
    return 0;
}
