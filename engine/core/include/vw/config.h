#pragma once

#ifndef VW_CONFIG_H
#define VW_CONFIG_H

// The single traditional header of the engine: build-time switches that have to
// be macros. Everything else is reached through `import`.
//
// Log levels: 0 trace, 1 debug, 2 info, 3 warn, 4 error, 5 critical, 6 off.
// Calls below VW_LOG_MIN_LEVEL are discarded at compile time, so raising it
// removes both the call and the argument formatting from the binary.
#ifndef VW_LOG_MIN_LEVEL
#  define VW_LOG_MIN_LEVEL 0
#endif

#endif  // VW_CONFIG_H
