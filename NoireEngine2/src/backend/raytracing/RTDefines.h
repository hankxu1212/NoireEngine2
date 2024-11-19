#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
#pragma once
#endif

///// This part should be shared between CPU and GPU. /////
#define INSTANCE_OPAQUE (1 << 0)
#define INSTANCE_TRANSLUCENT (1 << 1)
#define INSTANCE_IS_SOMETHING_ELSE (1 << 2)
