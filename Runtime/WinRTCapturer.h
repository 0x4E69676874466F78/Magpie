#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"
#include "D2DContext.h"

#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include <Windows.Graphics.Capture.Interop.h>



namespace winrt {

using namespace Windows::Foundation;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;

}


// ʹ�� Window Runtime �� Windows.Graphics.Capture API ץȡ����
class WinRTCapturer : public WindowCapturerBase {
public:
	WinRTCapturer(
		HWND hwndSrc,
		const RECT& srcRect,
		D2DContext& d2dContext
	) : _srcRect(srcRect), _hwndSrc(hwndSrc), _d2dContext(d2dContext),
		_captureFramePool(nullptr), _captureSession(nullptr), _captureItem(nullptr), _wrappedDevice(nullptr)
	{
		winrt::init_apartment(winrt::apartment_type::multi_threaded);
		// ���´���ο��� http://tips.hecomi.com/entry/2021/03/23/230947

		ID3D11Device* d3dDevice = d2dContext.GetD3DDevice();
		ComPtr<IDXGIDevice> dxgiDevice;
		Debug::ThrowIfComFailed(
			d3dDevice->QueryInterface<IDXGIDevice>(&dxgiDevice),
			L"��ȡ DXGI Device ʧ��"
		);

		Debug::ThrowIfComFailed(
			CreateDirect3D11DeviceFromDXGIDevice(
				dxgiDevice.Get(),
				reinterpret_cast<::IInspectable**>(winrt::put_abi(_wrappedDevice))
			),
			L"��ȡ IDirect3DDevice ʧ��"
		);
		Debug::Assert(_wrappedDevice, L"���� IDirect3DDevice ʧ��");
		
		auto interop = winrt::get_activation_factory<winrt::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
		Debug::ThrowIfComFailed(
			interop->CreateForWindow
			(
				hwndSrc,
				winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
				winrt::put_abi(_captureItem)
			),
			L"���� GraphicsCaptureItem ʧ��"
		);
		Debug::Assert(_captureItem, L"���� GraphicsCaptureItem ʧ��");

		_captureFramePool = winrt::Direct3D11CaptureFramePool::Create(
			_wrappedDevice, // D3D device
			winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized, // Pixel format
			1, // Number of frames
			_captureItem.Size() // Size of the buffers
		);
		Debug::Assert(_captureFramePool, L"���� Direct3D11CaptureFramePool ʧ��");

		_captureSession = _captureFramePool.CreateCaptureSession(_captureItem);
		Debug::Assert(_captureSession, L"CreateCaptureSession ʧ��");
		_captureSession.IsCursorCaptureEnabled(false);

		_captureSession.StartCapture();
	}

	~WinRTCapturer() {
		if (_captureSession) {
			_captureSession.Close();
		}
		if (_captureFramePool) {
			_captureFramePool.Close();
		}

		winrt::uninit_apartment();
	}

	std::variant<ComPtr<IWICBitmapSource>, ComPtr<ID2D1Bitmap1>> GetFrame() override {
		winrt::Direct3D11CaptureFrame frame = _captureFramePool.TryGetNextFrame();
		if (!frame) {
			return ComPtr<ID2D1Bitmap1>();
		}
		winrt::IDirect3DSurface d3dSurface = frame.Surface();

		winrt::com_ptr<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess> dxgiInterfaceAccess(
			d3dSurface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>()
		);
		winrt::com_ptr<::IDXGISurface> dxgiSurface;
		Debug::ThrowIfComFailed(
			dxgiInterfaceAccess->GetInterface(
				__uuidof(dxgiSurface),
				dxgiSurface.put_void()
			),
			L"�ӻ�ȡ IDirect3DSurface ��ȡ IDXGISurface ʧ��"
		);

		ComPtr<ID2D1Bitmap1> bmp;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateBitmapFromDxgiSurface(dxgiSurface.get(), BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE, PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &bmp),
			L""
		);
		return bmp;
	}

private:
	const RECT& _srcRect;
	HWND _hwndSrc;
	D2DContext& _d2dContext;

	winrt::Direct3D11CaptureFramePool _captureFramePool;
	winrt::GraphicsCaptureSession _captureSession;
	winrt::GraphicsCaptureItem _captureItem;
	winrt::IDirect3DDevice _wrappedDevice;
};
