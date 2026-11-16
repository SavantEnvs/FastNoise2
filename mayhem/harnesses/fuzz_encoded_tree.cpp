// Fuzzes FastNoise::NewFromEncodedNodeTree with the raw text of an encoded node tree (the string
// the Node Editor produces), then generates noise from whatever tree it built.
#include <cstddef>
#include <cstdint>
#include <string>

#include "exercise.h"

extern "C" int LLVMFuzzerTestOneInput( const uint8_t* data, size_t size )
{
    const std::string encoded( reinterpret_cast<const char*>( data ), size );

    FastNoise::SmartNode<> node = FastNoise::NewFromEncodedNodeTree( encoded.c_str() );
    if( !node )
    {
        return -1;
    }

    ExerciseNode( node );
    return 0;
}
