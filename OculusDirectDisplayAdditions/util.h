#pragma once
#include <windows.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <vector>
#include <cstdint>

class Util {
public:
  static uint64_t GetPrimaryAdapterLuid() {
    IDXGIFactory* dxgiFactory;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgiFactory));

    IDXGIAdapter* dxgiAdapter;
    hr = dxgiFactory->EnumAdapters(0, &dxgiAdapter);
    DXGI_ADAPTER_DESC adapterDesc;
    dxgiAdapter->GetDesc(&adapterDesc);

    dxgiAdapter->Release();
    dxgiFactory->Release();

    ULARGE_INTEGER result;
    result.LowPart = adapterDesc.AdapterLuid.LowPart;
    result.HighPart = adapterDesc.AdapterLuid.HighPart;
    return result.QuadPart;
  }
};
