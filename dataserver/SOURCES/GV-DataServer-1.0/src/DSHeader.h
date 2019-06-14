#ifndef __ADDR_H__
#define __ADDR_H__
#include "OSHeaders.h"
enum
{
    FatalVerbosity              = 0,
    WarningVerbosity            = 1,
    MessageVerbosity            = 2,
    AssertVerbosity             = 3,
    DebugVerbosity              = 4,
    IllegalVerbosity            = 5
};
typedef UInt32 ADDR_ErrorVerbosity;
#endif