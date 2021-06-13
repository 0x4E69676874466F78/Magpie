#pragma once
#include "pch.h"
#include "Utils.h"
#include "Env.h"

using namespace D2D1;


// Direct2D ����
class D2DContext {
public:
	D2DContext(bool noVsync = false): _noVSync(noVsync) {
        _InitD2D();
	}
 
    // ���ɸ��ƣ������ƶ�
    D2DContext(const D2DContext&) = delete;
    D2DContext(D2DContext&&) = delete;

    ~D2DContext() {
        CloseHandle(_frameLatencyWaitableObject);
    }

    void Render(std::function<void(ID2D1DeviceContext*)> renderFunc) {
        // GDI ����Ҫ�����ȴ���һ�δ�ֱͬ��
        Debug::ThrowIfComFailed(
            Env::$instance->GetDxgiOutput()->WaitForVBlank(),
            L"WaitForVBlankʧ��"
        );
        //WaitForSingleObjectEx(_frameLatencyWaitableObject, 1000, true);

        _d2dDC->BeginDraw();
        renderFunc(_d2dDC.Get());
        Debug::ThrowIfComFailed(
            _d2dDC->EndDraw(),
            L"EndDraw ʧ��"
        );
        
        Debug::ThrowIfComFailed(
            _dxgiSwapChain->Present(0, _noVSync ? DXGI_PRESENT_ALLOW_TEARING : 0),
            L"Present ʧ��"
        );
    }

private:
    void _InitD2D() {
        ComPtr<ID3D11Device> d3dDevice = nullptr;
        ComPtr<ID2D1Factory1> d2dFactory = nullptr;
        ComPtr<ID2D1Device> d2dDevice = nullptr;
        

        // This array defines the set of DirectX hardware feature levels this app  supports.
        // The ordering is important and you should  preserve it.
        // Don't forget to declare your app's minimum required feature level in its
        // description.  All apps are assumed to support 9.1 unless otherwise stated.
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };

        // Create the DX11 API device object, and get a corresponding context.
        ComPtr<ID3D11DeviceContext> d3dDC = nullptr;

        D3D_FEATURE_LEVEL fl;
        Debug::ThrowIfComFailed(
            D3D11CreateDevice(
                nullptr,    // specify null to use the default adapter
                D3D_DRIVER_TYPE_HARDWARE,
                NULL,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                featureLevels,  // list of feature levels this app can support
                ARRAYSIZE(featureLevels),   // number of possible feature levels
                D3D11_SDK_VERSION,
                &d3dDevice, // returns the Direct3D device created
                &fl,    // returns feature level of device created
                &d3dDC  // returns the device immediate context
            ),
            L"���� D3D Device ʧ��"
        );

        // Obtain the underlying DXGI device of the Direct3D11 device.
        ComPtr<IDXGIDevice1> dxgiDevice;
        Debug::ThrowIfComFailed(
            d3dDevice.As(&dxgiDevice),
            L"��ȡ DXGI Device ʧ��"
        );

        Debug::ThrowIfComFailed(
            D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(&d2dFactory)),
            L"���� D2D Factory ʧ��"
        );

        // Obtain the Direct2D device for 2-D rendering.
        Debug::ThrowIfComFailed(
            d2dFactory->CreateDevice(dxgiDevice.Get(), &d2dDevice),
            L"���� D2D Device ʧ��"
        );
        
        // Get Direct2D device's corresponding device context object.
        Debug::ThrowIfComFailed(
            d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_d2dDC),
            L"���� D2D DC ʧ��"
        );

        // Identify the physical adapter (GPU or card) this device is runs on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        Debug::ThrowIfComFailed(
            dxgiDevice->GetAdapter(&dxgiAdapter),
            L"��ȡ DXGI Adapter ʧ��"
        );

        // Get the factory object that created the DXGI device.
        ComPtr<IDXGIFactory2> dxgiFactory = nullptr;
        Debug::ThrowIfComFailed(
            dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)),
            L"��ȡ DXGI Factory ʧ��"
        );
        
        // Allocate a descriptor.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.Flags = (_noVSync ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) 
            | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        const RECT& hostClient = Env::$instance->GetHostClient();
        swapChainDesc.Width = hostClient.right - hostClient.left,
        swapChainDesc.Height = hostClient.bottom - hostClient.top,
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // this is the most common swapchain format
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;                // don't use multi-sampling
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;                     // use double buffering to enable flip
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        // Get the final swap chain for this window from the DXGI factory.
        ComPtr<IDXGISwapChain1> dxgiSwapChain1;
        Debug::ThrowIfComFailed(
            dxgiFactory->CreateSwapChainForHwnd(
                d3dDevice.Get(),
                Env::$instance->GetHwndHost(),
                &swapChainDesc,
                nullptr,
                nullptr,
                &dxgiSwapChain1
            ),
            L"���� Swap Chain ʧ��"
        );
        // ת���� IDXGISwapChain2 ��ʹ�� DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
        // �� https://docs.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_chain_flag
        Debug::ThrowIfComFailed(
            dxgiSwapChain1.As(&_dxgiSwapChain),
            L"��ȡ IDXGISwapChain2 ʧ��"
        );
        
        Debug::ThrowIfComFailed(
            _dxgiSwapChain->SetMaximumFrameLatency(Env::$instance->IsLowLantencyMode() ? 1 : 2),
            L"SetMaximumFrameLatency ʧ��"
        );

        _frameLatencyWaitableObject = _dxgiSwapChain->GetFrameLatencyWaitableObject();

        // Direct2D needs the dxgi version of the backbuffer surface pointer.
        ComPtr<IDXGISurface> dxgiBackBuffer = nullptr;
        Debug::ThrowIfComFailed(
            _dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer)),
            L"��ȡ DXGI Backbuffer ʧ��"
        );

        // Now we set up the Direct2D render target bitmap linked to the swapchain. 
        // Whenever we render to this bitmap, it is directly rendered to the 
        // swap chain associated with the window.
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
        );

        // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
        ComPtr<ID2D1Bitmap1> d2dTargetBitmap = nullptr;
        Debug::ThrowIfComFailed(
            _d2dDC->CreateBitmapFromDxgiSurface(
                dxgiBackBuffer.Get(),
                &bitmapProperties,
                &d2dTargetBitmap
            ),
            L"CreateBitmapFromDxgiSurface ʧ��"
        );

        // Now we can set the Direct2D render target.
        _d2dDC->SetTarget(d2dTargetBitmap.Get());
        _d2dDC->SetUnitMode(D2D1_UNIT_MODE_PIXELS);

        Env::$instance->SetD2DContext(d3dDevice, d2dFactory, d2dDevice, _d2dDC, _dxgiSwapChain);
    }

    bool _noVSync;

    ComPtr<ID2D1DeviceContext> _d2dDC = nullptr;
    ComPtr<IDXGISwapChain2> _dxgiSwapChain = nullptr;

    HANDLE _frameLatencyWaitableObject;
};
