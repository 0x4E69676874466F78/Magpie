#pragma once
#include "pch.h"
#include "EffectRendererBase.h"
#include "Env.h"


// ����Ϊ IWICBitmapSource
class WICBitmapEffectRenderer : public EffectRendererBase {
public:
	WICBitmapEffectRenderer() {
		Debug::ThrowIfComFailed(
			Env::$instance->GetD2DDC()->CreateEffect(CLSID_D2D1BitmapSource, &_d2dSourceEffect),
			L"���� D2D1BitmapSource ʧ��"
		);
		_outputEffect = _d2dSourceEffect;

		_Init();
	}

	void SetInput(ComPtr<IUnknown> inputImg) override {
		ComPtr<IWICBitmapSource> wicBitmap;
		Debug::ThrowIfComFailed(
			inputImg.As<IWICBitmapSource>(&wicBitmap),
			L"��ȡ����ͼ��ʧ��"
		);

		Debug::ThrowIfComFailed(
			_d2dSourceEffect->SetValue(D2D1_BITMAPSOURCE_PROP_WIC_BITMAP_SOURCE, wicBitmap.Get()),
			L"���� D2D1BitmapSource Դʧ��"
		);
	}

protected:
	void _PushAsOutputEffect(ComPtr<ID2D1Effect> effect) override {
		effect->SetInputEffect(0, _outputEffect.Get());
		_outputEffect = effect;
	}

	ComPtr<ID2D1Image> _GetOutputImg() override {
		ComPtr<ID2D1Image> outputImg = nullptr;
		_outputEffect->GetOutput(&outputImg);

		return outputImg;
	}

private:
	ComPtr<ID2D1Effect> _d2dSourceEffect = nullptr;
	ComPtr<ID2D1Effect> _outputEffect = nullptr;
};
