#pragma once
#include <FastNoise/FastNoise.h>

// Drive a decoded node tree through the generation entry points on small, exactly-sized buffers.
inline void ExerciseNode( const FastNoise::SmartNode<>& node )
{
    float grid2d[8 * 8];
    node->GenUniformGrid2D( grid2d, 0.0f, 0.0f, 8, 8, 1.0f, 1.0f, 1337 );

    float grid3d[4 * 4 * 4];
    node->GenUniformGrid3D( grid3d, 0.0f, 0.0f, 0.0f, 4, 4, 4, 1.0f, 1.0f, 1.0f, 1337 );

    volatile float single = node->GenSingle3D( 0.5f, 0.5f, 0.5f, 1337 );
    (void)single;
}
