#pragma once
#include "pch.h"
#include "EffectRendererBase.h"


// ����Ϊ D2D1Image
class D2DImageEffectRenderer : public EffectRendererBase {
public:
	D2DImageEffectRenderer(
		D2DContext& d2dContext,
		const std::wstring_view& effectsJson,
		const SIZE& srcSize,
		const RECT& hostClient
	) : EffectRendererBase(d2dContext, effectsJson, srcSize, hostClient) {
		assert(srcSize.cx > 0 && srcSize.cy > 0);

		_Init(effectsJson, srcSize);
	}

	void SetInput(ComPtr<IUnknown> inputImg) override {
		Debug::ThrowIfComFailed(
			inputImg.As<ID2D1Image>(&_inputImg),
			L"��ȡ����ͼ��ʧ��"
		);
	}
	
protected:
	void _PushAsOutputEffect(ComPtr<ID2D1Effect> effect) override {
		if (_firstEffect) {
			effect->SetInputEffect(0, _outputEffect.Get());
			_outputEffect = effect;
		} else {
			_outputEffect = _firstEffect = effect;
		}
	}

	ComPtr<ID2D1Image> _GetOutputImg() override {
		if (_firstEffect) {
			ComPtr<ID2D1Image> outputImg;
			_firstEffect->SetInput(0, _inputImg.Get());
			_outputEffect->GetOutput(&outputImg);

			return outputImg;
		} else {
			return _inputImg;
		}
	}

private:
	ComPtr<ID2D1Image> _inputImg = nullptr;

	ComPtr<ID2D1Effect> _firstEffect = nullptr;
	ComPtr<ID2D1Effect> _outputEffect = nullptr;
};
