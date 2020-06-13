// Trinket Game Engine
// (C) 2020 Max Kaufmann <max.kaufmann@gmail.com>

#include "Math.h"

//static_assert( sizeof(HPose) == sizeof(HPoseWithFlags) );
static_assert( std::alignment_of<quat>::value == 4);
static_assert( std::alignment_of<vec3>::value == 4);