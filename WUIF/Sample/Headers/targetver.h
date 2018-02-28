#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#include <WinSDKVer.h>
//Win7 Platform Update were included in Win8 SDK headers, so minimum version is Win8 (0x0602)
#define _WIN7_PLATFORM_UPDATE
#define _WIN32_WINNT  0x0602
#define WINVER        0x0602
#define NTDDI_VERSION 0x06020000
#include <SDKDDKVer.h>