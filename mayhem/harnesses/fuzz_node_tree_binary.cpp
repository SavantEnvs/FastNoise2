// Fuzzes the binary node-tree stream directly: the input bytes are the serialised tree, which is
// Base64-encoded with the library's own encoder and handed to NewFromEncodedNodeTree. This reaches
// the node/member deserialiser without the fuzzer having to invent valid Base64 text first, and
// checks the library's encoder and decoder agree on every byte string.
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "FastNoise/Base64.h"
#include "exercise.h"

extern "C" int LLVMFuzzerTestOneInput( const uint8_t* data, size_t size )
{
    const std::vector<uint8_t> stream( data, data + size );
    const std::string encoded = FastNoise::Base64::Encode( stream );

    if( FastNoise::Base64::Decode( encoded.c_str() ) != stream )
    {
        abort();
    }

    FastNoise::SmartNode<> node = FastNoise::NewFromEncodedNodeTree( encoded.c_str() );
    if( !node )
    {
        return -1;
    }

    ExerciseNode( node );
    return 0;
}
