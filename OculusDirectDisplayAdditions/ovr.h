#pragma once
#include <windows.h>
#include <cstdint>
#include <cstdio>

#if defined(_WIN32)
#define OVR_EXPORT extern "C" __declspec(dllexport)
#else
#error "Unsupported Platform."
#endif

typedef struct ovrSizei_ {
  int w, h;
} ovrSizei;

typedef struct ovrRational_ {
  int Numerator;
  int Denominator;
} ovrRational;

typedef char ovrBool;

#define ovrFalse 0
#define ovrTrue 1

typedef int32_t ovrResult;

typedef enum ovrSuccessType_ {
  ovrSuccess = 0
} ovrSuccessType;

typedef enum ovrErrorType_ {
  ovrError_InvalidParameter = -1005,
  ovrError_Unsupported = -1009
} ovrErrorType;

typedef enum ovrTextureFormat_ {
  OVR_FORMAT_UNKNOWN = 0,
  OVR_FORMAT_B5G6R5_UNORM = 1,
  OVR_FORMAT_B5G5R5A1_UNORM = 2,
  OVR_FORMAT_B4G4R4A4_UNORM = 3,
  OVR_FORMAT_R8G8B8A8_UNORM = 4,
  OVR_FORMAT_R8G8B8A8_UNORM_SRGB = 5,
  OVR_FORMAT_B8G8R8A8_UNORM = 6,
  OVR_FORMAT_B8G8R8_UNORM = 27, // Not supported.
  OVR_FORMAT_B8G8R8A8_UNORM_SRGB = 7,
  OVR_FORMAT_B8G8R8X8_UNORM = 8,
  OVR_FORMAT_B8G8R8X8_UNORM_SRGB = 9,
  OVR_FORMAT_R16G16B16A16_FLOAT = 10,
  OVR_FORMAT_R11G11B10_FLOAT = 25,
  OVR_FORMAT_R32_FLOAT = 26,
  OVR_FORMAT_D16_UNORM = 11,
  OVR_FORMAT_D24_UNORM_S8_UINT = 12,
  OVR_FORMAT_D32_FLOAT = 13,
  OVR_FORMAT_D32_FLOAT_S8X24_UINT = 14,
  OVR_FORMAT_BC1_UNORM = 15,
  OVR_FORMAT_BC1_UNORM_SRGB = 16,
  OVR_FORMAT_BC2_UNORM = 17,
  OVR_FORMAT_BC2_UNORM_SRGB = 18,
  OVR_FORMAT_BC3_UNORM = 19,
  OVR_FORMAT_BC3_UNORM_SRGB = 20,
  OVR_FORMAT_BC6H_UF16 = 21,
  OVR_FORMAT_BC6H_SF16 = 22,
  OVR_FORMAT_BC7_UNORM = 23,
  OVR_FORMAT_BC7_UNORM_SRGB = 24,
  OVR_FORMAT_LAST,
  OVR_FORMAT_ENUMSIZE = 0x7fffffff
} ovrTextureFormat;

typedef enum ovrDisplayOutputType_ {
  ovrDisplayOutput_Unknown = 0,
  ovrDisplayOutput_DP = 1,
  ovrDisplayOutput_DVI = 2,
  ovrDisplayOutput_HDMI = 3
} ovrDisplayOutputType;

typedef enum ovrDisplayDongleType_ {
  ovrDisplayDongle_Unknown = 0,
  ovrDisplayDongle_None = 1,
  ovrDisplayDongle_PassiveDPToDVI = 2,
  ovrDisplayDongle_PassiveDPToHDMI = 3,
  ovrDisplayDongle_ActiveDPToDVI = 4,
  ovrDisplayDongle_ActiveDPToHDMI = 5,
  ovrDisplayDongle_ActiveDPToVGA = 6
} ovrDisplayDongleType;

typedef struct ovrGraphicsLuid_ {
  char Reserved[8];
} ovrGraphicsLuid;

typedef struct ovrDisplayProperties_ {
  ovrGraphicsLuid Luid;
  ovrDisplayOutputType OutputType;
  ovrDisplayDongleType DongleType;
  uint64_t UniqueId;
  char EdidData[0x100];
} ovrDisplayProperties;

typedef struct ovrModeDesc_ {
  ovrSizei Resolution;
  ovrRational RefreshRate;
  ovrTextureFormat Format;
  char Reserved[4]; // TODO(Kaitlyn): Is this actually reserved?
} ovrModeDesc;

typedef struct ovrPresentStats_ {
  ovrRational RefreshRate;
  // TODO(Kaitlyn): Reverse all of this sometime, no clue what any of this is.
  // Maybe stuff from IDXGISwapChain::GetFrameStatistics?
  uint64_t Reserved1;
  uint64_t Reserved2;
  uint64_t Reserved3;
  uint64_t Reserved4;
  uint64_t Reserved5;
  uint64_t Reserved6;
} ovrPresentStats;

namespace OVRInterface {

class IRefCounted {
public:
  virtual ~IRefCounted() = default;

  virtual void AddRef() = 0;
  virtual void Release() = 0;

  // This isn't apart of standard COM, no idea what Oculus was thinking.
  // This also defeats the purpose of an interface, but whatever.
  virtual void Destroy() { delete this; }
};

// Oculus uses this abhorrent COM-like interface structure, pain.
class IUnknown : public IRefCounted {
public:
  virtual void* QueryInterface(uint64_t iid) = 0;
};

} // OVRInterface

namespace OVR {

static const uint64_t IID_IDirectDisplaySurface = 0xAD7152FE3248D8C6;
static const uint64_t IID_IDirectDisplay = 0xAD7152FE3248D8C6;
static const uint64_t IID_IDirectDisplayAPI = 0xAD7152FE3248D8C8;

class IDirectDisplaySurface : public OVRInterface::IUnknown {
public:
  virtual HANDLE GetSurfaceHandle() = 0;
};

class IDirectDisplay : public OVRInterface::IUnknown {
public:
  virtual ovrResult GetProperties(ovrDisplayProperties* outProperties) = 0;

  // No idea what this is, all of the IDirectDisplay implementations in OVRServer return unsupported.
  // So, we'll just follow the trend and return unsupported as well /shrug
  virtual ovrResult IDirectDisplay_Unk06() { return ovrError_Unsupported; }

  virtual ovrResult AcquireExclusive(::IUnknown* d3dPtr) = 0;
  virtual ovrResult ReleaseExclusive() = 0;

  // TODO(Kaitlyn): No clue what these 2 functions do, stubbing these. Not important.
  virtual ovrResult GetCurrentMode(void* a1) { return ovrError_Unsupported; }
  virtual ovrResult SetMode(void* a1) { return ovrSuccess; }

  virtual ovrResult CreateSurface(ovrModeDesc* modeDesc, IDirectDisplaySurface** outSurface) = 0;
  virtual ovrResult CreateFrameNotification(void* a1, void* a2) { return ovrError_Unsupported; } // Not required, so we'll just return unsupported.

  virtual ovrResult Present(IDirectDisplaySurface* surface, void* a2) = 0; // Still don't know what "a2" is, can be ignored for now I think?

  // Basically the same as GetPresentState, except the WaitForSingleObject passes in the custom timeout.
  // There is also no "outState" variable, the ovrResult is the state of the present state.
  // For example: ovrResult = ovrSuccess if the WaitForSingleObject is non-signaled.
  // Whatever else if there is a specific error when waiting for object.
  virtual ovrResult GetPresentStateTimeout(int timeoutMilliseconds) = 0;
  virtual ovrResult GetPresentStats(ovrPresentStats* outPresentStats) = 0;

  // TODO(Kaitlyn): These 2 functions have something to do with display visibility, no idea what they do.
  // Never called in practice, as far as I know... We'll just stub them, shouldn't matter.
  virtual ovrResult IDirectDisplay_Unk16(void* a1) { return ovrSuccess; }
  virtual ovrResult IDirectDisplay_Unk17() { return ovrSuccess; }

  virtual ovrResult GetPresentState(ovrBool* outState) = 0; // outValue: True = Present signaled (hangs rendering), False = Present non-signaled
  virtual ovrResult GetHdcpState(ovrBool* outState) = 0; // outValue: True = HDCP active, False = HDCP inactive
};

class IDirectDisplayAPI : public OVRInterface::IUnknown {
public:
  virtual ovrResult EnableDirectDisplay(ovrBool value) = 0;

  // TODO(Kaitlyn): Probably used for hotplugging, we'll stub this for now. Will come back to this later, hopefully.
  virtual ovrResult RegisterCallbacks(void* a1, void* a2) { return ovrSuccess; }

  // Used for getting displays by index? Never called in practice, stubbed as a result.
  virtual ovrResult IDirectDisplayAPI_Unk07(void* a1) { return ovrSuccess; }

  virtual ovrResult Enumerate(uint32_t index, IDirectDisplay** outDirectDisplay) = 0;
  virtual ovrResult GetName(char* outName) = 0;
};

} // OVR
