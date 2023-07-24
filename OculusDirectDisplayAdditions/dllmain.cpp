#include <windows.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <vector>
#include <thread>
#include <atomic>
#include <queue>
#include "ovr.h"
#include "util.h"

// HARDCODED EDID!! TODO
static const uint8_t GenericEdidData[0x100] = {
  0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
  0x3E, 0xD2, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0A, 0x19, 0x01, 0x03, 0x80, 0x00, 0x00, 0x78,
  0xE2, 0x60, 0xB1, 0xAA, 0x55, 0x40, 0xB6, 0x23,
  0x0C, 0x50, 0x54, 0x00, 0x00, 0x00, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xEB, 0x73,
  0x70, 0x50, 0x80, 0xB0, 0x10, 0x41, 0x08, 0x20,
  0x22, 0x0C, 0x77, 0x42, 0x00, 0x00, 0x00, 0x1A,
  0x00, 0x00, 0x00, 0xFC, 0x00, 0x52, 0x69, 0x66,
  0x74, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x57,
  0x4D, 0x48, 0x44, 0x33, 0x30, 0x33, 0x32, 0x32,
  0x30, 0x30, 0x43, 0x42, 0x00, 0x00, 0x00, 0xFD,
  0x00, 0x38, 0x78, 0x1E, 0x96, 0x20, 0x00, 0x0A,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x4B,
  0x02, 0x03, 0x0D, 0x00, 0x68, 0x03, 0x0C, 0x00,
  0x10, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B
};

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
  std::vector<HMONITOR>* monitors = reinterpret_cast<std::vector<HMONITOR>*>(dwData);
  monitors->push_back(hMonitor);
  return TRUE;
}

// TODO(Kaitlyn): Totally incorrect, we should search by display serial probably.
// For now, we're assuming the headset display is the second monitor.
HMONITOR GetSecondMonitor() {
  std::vector<HMONITOR> monitors;
  EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
  return monitors.size() >= 2 ? monitors[1] : NULL;
}

class AdditionsDirectDisplayRenderer {
private:
  HANDLE SurfaceHandle;
  ovrRational RefreshRate;
  ovrSizei Resolution;

  /* D3D11 rendering */

  HWND hWnd = {};

  ID3D11Device* d3d11Device = nullptr;
  ID3D11DeviceContext* d3d11DeviceContext = nullptr;

  ID3D11Texture2D* d3d11BackBuffer = nullptr;
  IDXGISwapChain* dxgiSwapChain = nullptr;
  ID3D11RenderTargetView* d3d11RenderTargetView = nullptr;

  std::thread RenderThread;

public:
  AdditionsDirectDisplayRenderer(HANDLE surfaceHandle, ovrRational refreshRate, ovrSizei resolution) {
    SurfaceHandle = surfaceHandle;
    RefreshRate = refreshRate;
    Resolution = resolution;

    Create();

    RenderThread = std::thread(&AdditionsDirectDisplayRenderer::RenderLoop, this);
    RenderThread.detach();
  }
  ~AdditionsDirectDisplayRenderer() {
    // ...
  }

private:
  void Create() {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProc;
    wc.hCursor = NULL;
    wc.lpszClassName = L"HeadsetView";
    RegisterClassEx(&wc);

    HMONITOR hMonitor = GetSecondMonitor();
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hMonitor, &monitorInfo);

    hWnd = CreateWindowEx(
      0,
      L"HeadsetView",
      L"Headset View",
      WS_OVERLAPPEDWINDOW,
      monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
      monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
      NULL,
      NULL,
      NULL,
      NULL);

    //ShowWindow(hWnd, 1);

    // Create D3D11 device and swap chain.
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width = Resolution.w;
    swapChainDesc.BufferDesc.Height = Resolution.h;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = RefreshRate.Numerator;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = RefreshRate.Denominator;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.Windowed = FALSE;

    D3D11CreateDeviceAndSwapChain(
      NULL,
      D3D_DRIVER_TYPE_HARDWARE,
      NULL,
      0,
      NULL,
      0,
      D3D11_SDK_VERSION,
      &swapChainDesc,
      &dxgiSwapChain,
      &d3d11Device,
      NULL,
      &d3d11DeviceContext);

    dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&d3d11BackBuffer));
    d3d11Device->CreateRenderTargetView(d3d11BackBuffer, NULL, &d3d11RenderTargetView);
    d3d11BackBuffer->Release();

    d3d11DeviceContext->OMSetRenderTargets(1, &d3d11RenderTargetView, NULL);
  }

  void RenderLoop() {
    MSG msg = {};
    while (WM_QUIT != msg.message) {
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      } else {
        ID3D11Texture2D* sharedTexture;
        d3d11Device->OpenSharedResource(SurfaceHandle, IID_PPV_ARGS(&sharedTexture));
        dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&d3d11BackBuffer));
        d3d11DeviceContext->CopyResource(d3d11BackBuffer, sharedTexture);

        dxgiSwapChain->Present(1, 0);
      }
    }
  }
};

class AdditionsDirectDisplaySurface : public OVR::IDirectDisplaySurface {
  OVR_REF_COUNTED_IMPLEMENTATION

private:
  AdditionsDirectDisplayRenderer* Renderer;
  HANDLE SurfaceHandle;

public:
  AdditionsDirectDisplaySurface(HANDLE surfaceHandle, ovrRational refreshRate, ovrSizei resolution) {
    Renderer = new AdditionsDirectDisplayRenderer(surfaceHandle, refreshRate, resolution);
    SurfaceHandle = surfaceHandle;
  }
  virtual ~AdditionsDirectDisplaySurface() {
    delete Renderer;
  }

  void* QueryInterface(uint64_t iid) {
    if (iid == OVR::IID_IDirectDisplaySurface || iid - 1 <= 1) {
      return (OVR::IDirectDisplaySurface*)this;
    }
    return nullptr;
  }

  HANDLE GetSurfaceHandle() {
    return SurfaceHandle;
  }
};

class AdditionsDirectDisplay : public OVR::IDirectDisplay {
  OVR_REF_COUNTED_IMPLEMENTATION

private:
  AdditionsDirectDisplaySurface* Surface = nullptr;
  ovrRational RefreshRate = {};

public:
  virtual ~AdditionsDirectDisplay() {}

  void* QueryInterface(uint64_t iid) {
    if (iid == OVR::IID_IDirectDisplay || iid - 1 <= 1) {
      return (OVR::IDirectDisplay*)this;
    }
    return nullptr;
  }

  ovrResult GetProperties(ovrDisplayProperties* outDisplayProperties) {
    memset(outDisplayProperties, 0, sizeof(ovrDisplayProperties));
    *(uint64_t*)outDisplayProperties->Luid.Reserved = Util::GetPrimaryAdapterLuid(); // This is required.
    outDisplayProperties->OutputType = ovrDisplayOutput_HDMI;
    outDisplayProperties->DongleType = ovrDisplayDongle_None;
    memcpy(outDisplayProperties->EdidData, GenericEdidData, 0x100);
    return ovrSuccess;
  }

  ovrResult AcquireExclusive(::IUnknown* d3dPtr) {
    return ovrSuccess;
  }

  ovrResult ReleaseExclusive() {
    return ovrSuccess;
  }

  ovrResult CreateSurface(ovrModeDesc* modeDesc, OVR::IDirectDisplaySurface** outSurface) {
    RefreshRate = modeDesc->RefreshRate;
    if (!Surface) {
      IDXGIFactory* dxgiFactory;
      HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgiFactory));

      IDXGIAdapter* dxgiAdapter;
      hr = dxgiFactory->EnumAdapters(0, &dxgiAdapter);
      DXGI_ADAPTER_DESC adapterDesc;
      dxgiAdapter->GetDesc(&adapterDesc);

      ID3D11Device* d3d11Device;
      D3D_FEATURE_LEVEL featureLevel;
      hr = D3D11CreateDevice(dxgiAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &d3d11Device, &featureLevel, nullptr);

      dxgiAdapter->Release();
      dxgiFactory->Release();

      D3D11_TEXTURE2D_DESC sharedTextureDesc = {};
      sharedTextureDesc.Width = modeDesc->Resolution.w;
      sharedTextureDesc.Height = modeDesc->Resolution.h;
      sharedTextureDesc.MipLevels = 1;
      sharedTextureDesc.ArraySize = 1;
      sharedTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      sharedTextureDesc.SampleDesc.Count = 1;
      sharedTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;
      sharedTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

      ID3D11Texture2D* sharedTexture;
      hr = d3d11Device->CreateTexture2D(&sharedTextureDesc, NULL, &sharedTexture);
      IDXGIResource* dxgiResource;
      hr = sharedTexture->QueryInterface(__uuidof(IDXGIResource), reinterpret_cast<void**>(&dxgiResource));
      HANDLE sharedHandle;
      hr = dxgiResource->GetSharedHandle(&sharedHandle);

      Surface = new AdditionsDirectDisplaySurface(sharedHandle, modeDesc->RefreshRate, modeDesc->Resolution);
    }

    *outSurface = Surface;
    (*outSurface)->AddRef();
    (*outSurface)->Release();
    return ovrSuccess;
  }

  ovrResult Present(OVR::IDirectDisplaySurface* surface, void* a2) {
    return ovrSuccess;
  }

  ovrResult GetPresentStateTimeout(int timeoutMilliseconds) {
    return ovrSuccess;
  }

  ovrResult GetPresentStats(ovrPresentStats* outPresentStats) {
    memset(outPresentStats, 0, sizeof(ovrPresentStats));
    outPresentStats->RefreshRate = RefreshRate;
    return ovrSuccess;
  }

  ovrResult GetPresentState(ovrBool* outState) {
    *outState = ovrFalse;
    return ovrSuccess;
  }

  ovrResult GetHdcpState(ovrBool* outState) {
    *outState = ovrTrue;
    return ovrSuccess;
  }
};

class AdditionsDirectDisplayAPI : public OVR::IDirectDisplayAPI {
  OVR_REF_COUNTED_IMPLEMENTATION

private:
  std::queue<AdditionsDirectDisplay*> AttachedDisplays;

public:
  AdditionsDirectDisplayAPI() {
    AttachedDisplays.push(new AdditionsDirectDisplay); // TODO...
  }
  virtual ~AdditionsDirectDisplayAPI() {}

  void* QueryInterface(uint64_t iid) {
    if (iid == OVR::IID_IDirectDisplayAPI || iid - 1 <= 1) {
      return (OVR::IDirectDisplayAPI*)this;
    }
    return nullptr;
  }

  ovrResult EnableDirectDisplay(ovrBool value) { return ovrSuccess; } // TODO(Kaitlyn)

  ovrResult Enumerate(uint32_t index, OVR::IDirectDisplay** outDirectDisplay) {
    if (!AttachedDisplays.empty()) {
      *outDirectDisplay = (OVR::IDirectDisplay*)AttachedDisplays.front()->QueryInterface(OVR::IID_IDirectDisplay);
      (*outDirectDisplay)->AddRef();
      (*outDirectDisplay)->Release();
      AttachedDisplays.pop();
      return ovrSuccess;
    } else {
      *outDirectDisplay = nullptr;
      return -1; // FIXME!! Not an actual ovrError.
    }
  }

  ovrResult GetName(char* outName) {
    if (!outName) return ovrError_InvalidParameter;
    strncpy_s(outName, 128, "Additions", ULONG_MAX);
    return ovrSuccess;
  }
};

OVR_EXPORT ovrResult DirectDisplayInitialize(uint64_t iid, OVR::IDirectDisplayAPI** outDirectDisplayApi) {
  if (iid == OVR::IID_IDirectDisplayAPI || iid - 1 <= 1) {
    *outDirectDisplayApi = new AdditionsDirectDisplayAPI;
    (*outDirectDisplayApi)->AddRef();
    (*outDirectDisplayApi)->Release();
    return ovrSuccess;
  }
  *outDirectDisplayApi = nullptr;
  return ovrError_InvalidParameter;
}
