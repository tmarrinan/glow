/**********************************************************
 *****                                                *****
 *****   GLOW: GL Object-oriented Windowing toolkit   *****
 *****                                                *****
 **********************************************************/

#ifndef __GLOW_H__
#define __GLOW_H__

#ifdef __APPLE__
#include "mac/glow.h"
#elif _WIN32
#include "windows/glow.h"
#else
#include "linux/glow.h"
#endif

#endif // __GLOW_H__
