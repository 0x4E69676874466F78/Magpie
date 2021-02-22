#pragma once
#include "pch.h"
#include "AdaptiveSharpenEffect.h"
#include "Anime4KUpscaleEffect.h"
#include "Anime4KUpscaleDeblurEffect.h"
#include "ScaleEffect.h"
#include "json.hpp"

class EffectManager {
public:
	EffectManager(
		ComPtr<ID2D1Factory1> d2dFactory, 
		ComPtr<ID2D1DeviceContext> d2dDC,
		const std::wstring_view& effectsJson,
		const SIZE &srcSize,
		const SIZE &maxSize
	): _destSize(srcSize), _maxSize(maxSize), _d2dFactory(d2dFactory), _d2dDC(d2dDC) {
		assert(srcSize.cx > 0 && srcSize.cy > 0);
		assert(maxSize.cx > 0 && maxSize.cy > 0);
		assert(d2dFactory != nullptr && d2dDC != nullptr);

		_RegisterEffects();
		_CreateSourceEffect();
		_ReadEffectsJson(effectsJson);
	}

	ComPtr<ID2D1Image> Apply(ComPtr<IWICBitmapSource> srcBmp) {
		assert(srcBmp != nullptr);

		Debug::ThrowIfFailed(
			_d2dSourceEffect->SetValue(D2D1_BITMAPSOURCE_PROP_WIC_BITMAP_SOURCE, srcBmp.Get()),
			L"���� D2D1BitmapSource Դʧ��"
		);

		ComPtr<ID2D1Image> outputImg = nullptr;
		_outputEffect->GetOutput(&outputImg);

		return outputImg;
	}

private:
	void _CreateSourceEffect() {
		// ���� Source effect
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_D2D1BitmapSource, &_d2dSourceEffect),
			L"���� D2D1BitmapSource ʧ��"
		);

		// ��ʼʱ���Ϊ Source effect
		_outputEffect = _d2dSourceEffect;
	}

	void _RegisterEffects() const {
		AdaptiveSharpenEffect::Register(_d2dFactory.Get());
		Anime4KUpscaleEffect::Register(_d2dFactory.Get());
		Anime4KUpscaleDeblurEffect::Register(_d2dFactory.Get());
		ScaleEffect::Register(_d2dFactory.Get());
	}

	void _ReadEffectsJson(const std::wstring_view& effectsJson) {
		const auto& effects = nlohmann::json::parse(effectsJson);
		Debug::ThrowIfFalse(effects.is_array(), L"json ��ʽ����");

		for (const auto &effect : effects) {
			Debug::ThrowIfFalse(effect.is_object(), L"json ��ʽ����");

			const auto &effectType = effect.value("effect", "");
			
			if (effectType == "scale") {
				const auto& subType = effect.value("type", "");

				if (subType == "anime4K") {
					//_AddAnime4KEffect();
					_AddAnime4KxDeblurEffect();
				} else if (subType == "jinc2") {
					_AddJinc2ScaleEffect(effect);
				}
			} else if (effectType == "sharpen") {
				const auto& subType = effect.value("type", "");

				if (subType == "adaptive") {
					_AddAdaptiveSharpenEffect();
					_AddBuiltInSharpenEffect();
				}
			} else {
				Debug::ThrowIfFalse(false, L"json ��ʽ����");
			}
		}

	}

	void _AddAdaptiveSharpenEffect() {
		ComPtr<ID2D1Effect> adaptiveSharpenEffect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT, &adaptiveSharpenEffect),
			L"���� Adaptive sharpen effect ʧ��"
		);

		// �滻 output effect
		adaptiveSharpenEffect->SetInputEffect(0, _outputEffect.Get());
		_outputEffect = adaptiveSharpenEffect;
	}

	void _AddBuiltInSharpenEffect() {
		ComPtr<ID2D1Effect> d2dSharpenEffect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_D2D1Sharpen, &d2dSharpenEffect),
			L"���� sharpen effect ʧ��"
		);

		d2dSharpenEffect->SetValue(D2D1_SHARPEN_PROP_SHARPNESS, 6.0f);
		d2dSharpenEffect->SetValue(D2D1_SHARPEN_PROP_THRESHOLD, 0.8f);

		// �滻 output effect
		d2dSharpenEffect->SetInputEffect(0, _outputEffect.Get());
		_outputEffect = d2dSharpenEffect;
	}

	void _AddAnime4KEffect() {
		ComPtr<ID2D1Effect> anime4KEffect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_MAGIPE_ANIME4K_UPSCALE_EFFECT, &anime4KEffect),
			L"���� Anime4K Effect ʧ��"
		);

		// �滻 output effect
		anime4KEffect->SetInputEffect(0, _outputEffect.Get());
		_outputEffect = anime4KEffect;

		// ���ͼ��ĳ��Ϳ��Ϊ 2 ��
		_destSize.cx *= 2;
		_destSize.cy *= 2;
	}

	void _AddAnime4KxDeblurEffect() {
		ComPtr<ID2D1Effect> anime4KxDeblurEffect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_MAGIPE_ANIME4K_UPSCALE_DEBLUR_EFFECT, &anime4KxDeblurEffect),
			L"���� Anime4K Effect ʧ��"
		);

		// �滻 output effect
		anime4KxDeblurEffect->SetInputEffect(0, _outputEffect.Get());
		_outputEffect = anime4KxDeblurEffect;

		// ���ͼ��ĳ��Ϳ��Ϊ 2 ��
		_destSize.cx *= 2;
		_destSize.cy *= 2;
	}

	void _AddJinc2ScaleEffect(const nlohmann::json& props) {
		ComPtr<ID2D1Effect> jinc2Effect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_SCALE_EFFECT, &jinc2Effect),
			L"���� Anime4K Effect ʧ��"
		);

		// scale ����
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scaleValues = *it;
			Debug::ThrowIfFalse(
				scaleValues.is_array() && scaleValues.size() == 2 
				&& scaleValues[0].is_number() && scaleValues[1].is_number(),
				L"��ȡ scale ����ʧ��"
			);

			D2D1_VECTOR_2F scale{ scaleValues[0], scaleValues[1]};

			if (scale.x == 0 || scale.y == 0) {
				// ���ͼ�������Ļ
				scale.x = min((FLOAT)_maxSize.cx / _destSize.cx, (FLOAT)_maxSize.cy / _destSize.cy);
				scale.y = scale.x;
			}
			
			Debug::ThrowIfFailed(
				jinc2Effect->SetValue(ScaleEffect::PROP_SCALE, scale),
				L"��ȡ scale ʧ��"
			);

			// ���� scale �����ͼ��ߴ�ı�
			_destSize.cx = lroundf(_destSize.cx * scale.x);
			_destSize.cy = lroundf(_destSize.cy * scale.y);
		}
		
		// �滻 output effect
		jinc2Effect->SetInputEffect(0, _outputEffect.Get());
		_outputEffect = jinc2Effect;
	}

	void _AddCubicScaleEffect() {
		ComPtr<ID2D1Effect> cubicEffect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_D2D1Scale, &cubicEffect),
			L"���� Anime4K Effect ʧ��"
		);

		cubicEffect->SetValue(D2D1_SCALE_PROP_INTERPOLATION_MODE, D2D1_SCALE_INTERPOLATION_MODE_ANISOTROPIC);
		cubicEffect->SetValue(D2D1_SCALE_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);

		float scale = min((FLOAT)_maxSize.cx / _destSize.cx, (FLOAT)_maxSize.cy / _destSize.cy);
		cubicEffect->SetValue(D2D1_SCALE_PROP_SCALE, D2D1_VECTOR_2F{ scale, scale });

		cubicEffect->SetInputEffect(0, _outputEffect.Get());
		_outputEffect = cubicEffect;
	}

	ComPtr<ID2D1Factory1> _d2dFactory;
	ComPtr<ID2D1DeviceContext> _d2dDC;

	ComPtr<ID2D1Effect> _d2dSourceEffect = nullptr;
	ComPtr<ID2D1Effect> _outputEffect = nullptr;

	// ���ͼ��ߴ�
	SIZE _destSize{};
	// ȫ�����ڳߴ�
	SIZE _maxSize;
};
