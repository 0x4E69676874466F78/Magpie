#pragma once
#include "pch.h"
#include "Utils.h"
#include "EffectManager.h"
#include "CursorManager.h"

using namespace D2D1;

class EffectRenderer {
public:
	EffectRenderer(
        HINSTANCE hInstance,
        HWND hwndHost,
        const std::wstring_view &effectsJson,
        const RECT& srcClient,
        const ComPtr<IWICImagingFactory2>& wicImgFactory
    ) {
        RECT destClient{};
        Debug::ThrowIfFalse(
            Utils::GetClientScreenRect(hwndHost, destClient),
            L"��ȡȫ�����ڳߴ�ʧ��"
        );
        _hostWndClientSize = Utils::GetSize(destClient);

        _InitD2D(hwndHost);

        _effectManager.reset(new EffectManager(_d2dFactory, _d2dDC, effectsJson, Utils::GetSize(srcClient), _hostWndClientSize));
        _cursorManager.reset(new CursorManager(hInstance, wicImgFactory, _d2dDC, srcClient));
	}
 
    // ���ɸ��ƣ������ƶ�
    EffectRenderer(const EffectRenderer&) = delete;
    EffectRenderer(EffectRenderer&&) = delete;
public:
    void Render(const ComPtr<IWICBitmapSource>& srcBmp) const {
        ComPtr<ID2D1Image> outputImg = _effectManager->Apply(srcBmp);

        // ��ȡ���ͼ��ߴ�
        D2D1_RECT_F outputRect{};
        Debug::ThrowIfFailed(
            _d2dDC->GetImageLocalBounds(outputImg.Get(), &outputRect),
            L"��ȡ���ͼ��ߴ�ʧ��"
        );

        D2D1_SIZE_F outputSize = Utils::GetSize(outputRect);
        D2D1_POINT_2F pos{
            ((float)_hostWndClientSize.cx - outputSize.width) / 2,
            ((float)_hostWndClientSize.cy - outputSize.height) / 2
        };


        // �����ͼ����ʾ�ڴ�������
        _d2dDC->BeginDraw();
        _d2dDC->Clear();
        _d2dDC->DrawImage(
            outputImg.Get(),
            pos
        );

        _cursorManager->DrawCursor(RECT{
            lround(outputRect.left + pos.x),
            lround(outputRect.top + pos.y),
            lround(outputRect.right + pos.x),
            lround(outputRect.bottom + pos.y)
        });

        Debug::ThrowIfFailed(
            _d2dDC->EndDraw(),
            L"EndDraw ʧ��"
        );

        Debug::ThrowIfFailed(
            _dxgiSwapChain->Present(0, 0),
            L"Present ʧ��"
        );
    }
private:
    void _InitD2D(HWND hwndHost) {
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
        ComPtr<ID3D11Device> d3dDevice = nullptr;
        ComPtr<ID3D11DeviceContext> d3dDC = nullptr;

        D3D_FEATURE_LEVEL fl;
        Debug::ThrowIfFailed(
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
        Debug::ThrowIfFailed(
            d3dDevice.As(&dxgiDevice),
            L"��ȡ DXGI Device ʧ��"
        );

        Debug::ThrowIfFailed(
            D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &_d2dFactory),
            L"���� D2D Factory ʧ��"
        );

        // Obtain the Direct2D device for 2-D rendering.
        
        Debug::ThrowIfFailed(
            _d2dFactory->CreateDevice(dxgiDevice.Get(), &_d2dDevice),
            L"���� D2D Device ʧ��"
        );
        
        // Get Direct2D device's corresponding device context object.
        Debug::ThrowIfFailed(
            _d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_d2dDC),
            L"���� D2D DC ʧ��"
        );

        // Identify the physical adapter (GPU or card) this device is runs on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        Debug::ThrowIfFailed(
            dxgiDevice->GetAdapter(&dxgiAdapter),
            L"��ȡ DXGI Adapter ʧ��"
        );

        // Get the factory object that created the DXGI device.
        ComPtr<IDXGIFactory2> dxgiFactory = nullptr;
        Debug::ThrowIfFailed(
            dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)),
            L"��ȡ DXGI Factory ʧ��"
        );
        
        // Allocate a descriptor.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.Width = _hostWndClientSize.cx,
        swapChainDesc.Height = _hostWndClientSize.cy,
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

        Debug::ThrowIfFailed(
            dxgiFactory->CreateSwapChainForHwnd(
                d3dDevice.Get(),
                hwndHost,
                &swapChainDesc,
                nullptr,
                nullptr,
                &_dxgiSwapChain
            ),
            L"���� Swap Chain ʧ��"
        );
        
        // Ensure that DXGI doesn't queue more than one frame at a time.
        Debug::ThrowIfFailed(
            dxgiDevice->SetMaximumFrameLatency(1),
            L"SetMaximumFrameLatency ʧ��"
        );

        // Direct2D needs the dxgi version of the backbuffer surface pointer.
        ComPtr<IDXGISurface> dxgiBackBuffer = nullptr;
        Debug::ThrowIfFailed(
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
        Debug::ThrowIfFailed(
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
    }


    SIZE _hostWndClientSize{};

    ComPtr<ID2D1Factory1> _d2dFactory = nullptr;
    ComPtr<ID2D1Device> _d2dDevice = nullptr;
    ComPtr<ID2D1DeviceContext> _d2dDC = nullptr;
    ComPtr<IDXGISwapChain1> _dxgiSwapChain = nullptr;

    std::unique_ptr<EffectManager> _effectManager = nullptr;
    std::unique_ptr<CursorManager> _cursorManager = nullptr;
};
