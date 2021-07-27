#pragma once
#include "pch.h"
#include "EffectRendererBase.h"
#include "Env.h"


// 输入为 IWICBitmapSource
class WICBitmapEffectRenderer : public EffectRendererBase {
public:
	WICBitmapEffectRenderer() {
		Debug::ThrowIfComFailed(
			Env::$instance->GetD2DDC()->CreateEffect(CLSID_D2D1BitmapSource, &_d2dSourceEffect),
			L"创建 D2D1BitmapSource 失败"
		);
		_outputEffect = _d2dSourceEffect;

		_Init();
	}

	ComPtr<ID2D1Image> Apply(IUnknown* inputImg) override {
		ComPtr<IWICBitmapSource> wicBitmap;
		Debug::ThrowIfComFailed(
			inputImg->QueryInterface<IWICBitmapSource>(&wicBitmap),
			L"获取输入图像失败"
		);

		Debug::ThrowIfComFailed(
			_d2dSourceEffect->SetValue(D2D1_BITMAPSOURCE_PROP_WIC_BITMAP_SOURCE, wicBitmap.Get()),
			L"设置 D2D1BitmapSource 源失败"
		);

		ComPtr<ID2D1Image> outputImg = nullptr;
		_outputEffect->GetOutput(&outputImg);

		return outputImg;
	}

protected:
	void _PushAsOutputEffect(ComPtr<ID2D1Effect> effect) override {
		effect->SetInputEffect(0, _outputEffect.Get());
		_outputEffect = effect;
	}

private:
	ComPtr<ID2D1Effect> _d2dSourceEffect = nullptr;
	ComPtr<ID2D1Effect> _outputEffect = nullptr;
};
