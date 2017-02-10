#pragma once

#include <Ns.h>
#include <Nv.h>

#if NV_WINDOWS_FAMILY
//#include <Windows/MinWindows.h>
#include <stdint.h>
#include <stdlib.h>
#elif NV_ANDROID
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <wctype.h>
#include <pthread.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <utime.h>
#endif

#include <stdint.h>
#include <type_traits>
#include <math.h>
#include <vector>
#include <assert.h>

#if NV_WINDOWS_FAMILY
#define GLEW_STATIC
#define GLFW_INCLUDE_ES2
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#elif NV_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif