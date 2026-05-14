// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// Use #pragma once to help compilers avoid multiple inclusion in a single
// translation unit. Kept for compatibility with the header guard above.
#pragma once

// Minimize inclusion of rarely-used APIs from windows.h to speed up build
// and reduce namespace pollution. Defines can be controlled here before
// including <windows.h>.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Core Win32 API header. Included here because many modules in the project
// rely on Win32 types and APIs.
#include <windows.h>

// BaseTsd.h provides fixed-width and pointer-size typedefs such as SSIZE_T.
// We include it to ensure SSIZE_T is available in this translation unit.
#include <BaseTsd.h>

// Provide a portable `ssize_t` alias on Windows by mapping to SSIZE_T.
// Some cross-platform code uses `ssize_t`, so this typedef avoids
// sprinkling platform-specific guards throughout the codebase.
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef SSIZE_T ssize_t;
#endif

// Third-party libraries used by the project:
//
// - ixwebsocket: lightweight WebSocket client/server. Including the server
//   header here makes the WebSocket server APIs available across the project.
//   Keep in mind: heavy third-party headers increase PCH size; consider moving
//   extremely volatile third-party headers out of the PCH if they change often.
#include <ixwebsocket/IXWebSocketServer.h>

// - httplib.h: single-header HTTP library (likely cpp-httplib). Included
//   project-wide for convenience where HTTP server/client features are needed.
#include "httplib.h"

// - framework.h: project-specific framework/common utilities. This is a good
//   place for foundational project headers that rarely change.
#include "framework.h"

#endif //PCH_H
