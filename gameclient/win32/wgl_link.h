#define f(x,y) x = (void*)wglGetProcAddress(#x);
#include "wgl_funcs.h"
#undef f
