#pragma once
#include "pch.h"
#include "Renderable.h"
#include "D2DContext.h"
#include "AdaptiveSharpenEffect.h"
#include "Anime4KUpscaleEffect.h"
#include "Anime4KUpscaleDeblurEffect.h"
#include "Anime4KUpscaleDenoiseEffect.h"
#include "Jinc2ScaleEffect.h"
#include "MitchellNetravaliScaleEffect.h"
#include "Lanczos6ScaleEffect.h"
#include "nlohmann/json.hpp"
#include <unordered_set>


class EffectManager : public Renderable {
public:
	EffectManager(
		D2DContext& d2dContext,
		const std::wstring_view& effectsJson,
		const SIZE &srcSize,
		const RECT& hostClient
	): Renderable(d2dContext), _hostClient(hostClient) {
		assert(srcSize.cx > 0 && srcSize.cy > 0);

		_SetDestSize(srcSize);
		_ReadEffectsJson(effectsJson);

		// �������λ�ã�x �� y ����Ϊ�����������ʹ����ģ��
		float x = float((_hostClient.right - _hostClient.left - _outputSize.cx) / 2);
		float y = float((_hostClient.bottom - _hostClient.top - _outputSize.cy) / 2);
		_outputRect = RectF(x, y, x + _outputSize.cx, y + _outputSize.cy);
	}

	// ���ɸ��ƣ������ƶ�
	EffectManager(const EffectManager&) = delete;
	EffectManager(EffectManager&&) = delete;


	void Render() override {
		if (_firstEffect) {
			_firstEffect->SetInput(0, _inputBmp.Get());

			ComPtr<ID2D1Image> outputImg = nullptr;
			_outputEffect->GetOutput(&outputImg);

			_d2dContext.GetD2DDC()->DrawImage(
				outputImg.Get(),
				Point2F(_outputRect.left, _outputRect.top)
			);
		} else {
			_d2dContext.GetD2DDC()->DrawImage(
				_inputBmp.Get(),
				Point2F(_outputRect.left, _outputRect.top)
			);
		}
	}

	void SetInput(ComPtr<ID2D1Bitmap> srcBmp) {
		assert(srcBmp != nullptr);

		_inputBmp = srcBmp;
	}

	const D2D1_RECT_F& GetOutputRect() const {
		return _outputRect;
	}

private:
	void _ReadEffectsJson(const std::wstring_view& effectsJson) {
		const auto& effects = nlohmann::json::parse(effectsJson);
		Debug::Assert(effects.is_array(), L"json ��ʽ����");

		for (const auto &effect : effects) {
			Debug::Assert(effect.is_object(), L"json ��ʽ����");

			const auto &effectType = effect.value("effect", "");
			
			if (effectType == "scale") {
				const auto& subType = effect.value("type", "");

				if (subType == "Anime4K") {
					_AddAnime4KEffect();
				} else if (subType == "Anime4KxDeblur") {
					_AddAnime4KxDeblurEffect();
				} else if (subType == "Anime4KxDenoise") {
					_AddAnime4KxDenoiseEffect();
				} else if (subType == "jinc2") {
					_AddJinc2ScaleEffect(effect);
				} else if (subType == "mitchell") {
					_AddMitchellNetravaliScaleEffect(effect);
				} else if (subType == "HQBicubic") {
					_AddHQBicubicScaleEffect(effect);
				} else if (subType == "lanczos6") {
					_AddLanczos6ScaleEffect(effect);
				} else {
					Debug::Assert(false, L"δ֪�� scale effect");
				}
			} else if (effectType == "sharpen") {
				const auto& subType = effect.value("type", "");

				if (subType == "adaptive") {
					_AddAdaptiveSharpenEffect(effect);
				} else if (subType == "builtIn") {
					_AddBuiltInSharpenEffect(effect);
				} else {
					Debug::Assert(false, L"δ֪�� sharpen effect");
				}
			} else {
				Debug::Assert(false, L"δ֪�� effect");
			}
		}
	}

	void _AddAdaptiveSharpenEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT,
			&AdaptiveSharpenEffect::Register
		);

		ComPtr<ID2D1Effect> adaptiveSharpenEffect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateEffect(CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT, &adaptiveSharpenEffect),
			L"���� Adaptive sharpen effect ʧ��"
		);

		// curveHeight ����
		auto it = props.find("curveHeight");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� curveHeight ����ֵ");

			float curveHeight = value.get<float>();
			Debug::Assert(
				curveHeight >= 0 && curveHeight <= 1,
				L"�Ƿ��� curveHeight ����ֵ"
			);

			Debug::ThrowIfComFailed(
				adaptiveSharpenEffect->SetValue(AdaptiveSharpenEffect::PROP_CURVE_HEIGHT, curveHeight),
				L"���� curveHeight ����ʧ��"
			);
		}

		// �滻 output effect
		_PushAsOutputEffect(adaptiveSharpenEffect);
	}

	void _AddBuiltInSharpenEffect(const nlohmann::json& props) {
		ComPtr<ID2D1Effect> d2dSharpenEffect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateEffect(CLSID_D2D1Sharpen, &d2dSharpenEffect),
			L"���� sharpen effect ʧ��"
		);

		// sharpness ����
		auto it = props.find("sharpness");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� sharpness ����ֵ");

			float sharpness = value.get<float>();
			Debug::Assert(
				sharpness >= 0 && sharpness <= 10,
				L"�Ƿ��� sharpness ����ֵ"
			);

			Debug::ThrowIfComFailed(
				d2dSharpenEffect->SetValue(D2D1_SHARPEN_PROP_SHARPNESS, sharpness),
				L"���� sharpness ����ʧ��"
			);
		}

		// threshold ����
		it = props.find("threshold");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� threshold ����ֵ");

			float threshold = value.get<float>();
			Debug::Assert(
				threshold >= 0 && threshold <= 1,
				L"�Ƿ��� threshold ����ֵ"
			);

			Debug::ThrowIfComFailed(
				d2dSharpenEffect->SetValue(D2D1_SHARPEN_PROP_THRESHOLD, threshold),
				L"���� threshold ����ʧ��"
			);
		}

		// �滻 output effect
		_PushAsOutputEffect(d2dSharpenEffect);
	}

	void _AddAnime4KEffect() {
		_CheckAndRegisterEffect(
			CLSID_MAGIPE_ANIME4K_UPSCALE_EFFECT,
			&Anime4KUpscaleEffect::Register
		);

		ComPtr<ID2D1Effect> anime4KEffect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateEffect(CLSID_MAGIPE_ANIME4K_UPSCALE_EFFECT, &anime4KEffect),
			L"���� Anime4K Effect ʧ��"
		);

		// ���ͼ��ĳ��Ϳ��Ϊ 2 ��
		_SetDestSize(SIZE{ _outputSize.cx * 2, _outputSize.cy * 2 });

		_PushAsOutputEffect(anime4KEffect);
	}

	void _AddAnime4KxDeblurEffect() {
		_CheckAndRegisterEffect(
			CLSID_MAGIPE_ANIME4K_UPSCALE_DEBLUR_EFFECT,
			&Anime4KUpscaleDeblurEffect::Register
		);

		ComPtr<ID2D1Effect> anime4KxDeblurEffect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateEffect(CLSID_MAGIPE_ANIME4K_UPSCALE_DEBLUR_EFFECT, &anime4KxDeblurEffect),
			L"���� Anime4K Effect ʧ��"
		);

		// ���ͼ��ĳ��Ϳ��Ϊ 2 ��
		_SetDestSize(SIZE{ _outputSize.cx * 2, _outputSize.cy * 2 });

		_PushAsOutputEffect(anime4KxDeblurEffect);
	}

	void _AddAnime4KxDenoiseEffect() {
		_CheckAndRegisterEffect(
			CLSID_MAGIPE_ANIME4K_UPSCALE_DENOISE_EFFECT,
			&Anime4KUpscaleDenoiseEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateEffect(CLSID_MAGIPE_ANIME4K_UPSCALE_DENOISE_EFFECT, &effect),
			L"���� Anime4K Effect ʧ��"
		);

		// ���ͼ��ĳ��Ϳ��Ϊ 2 ��
		_SetDestSize(SIZE{ _outputSize.cx * 2, _outputSize.cy * 2 });

		_PushAsOutputEffect(effect);
	}

	void _AddJinc2ScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_JINC2_SCALE_EFFECT,
			&Jinc2ScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateEffect(CLSID_MAGPIE_JINC2_SCALE_EFFECT, &effect),
			L"���� Anime4K Effect ʧ��"
		);
		

		// scale ����
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);
			
			Debug::ThrowIfComFailed(
				effect->SetValue(Jinc2ScaleEffect::PROP_SCALE, scale),
				L"���� scale ����ʧ��"
			);

			// ���� scale �����ͼ��ߴ�ı�
			_SetDestSize(SIZE{ lroundf(_outputSize.cx * scale.x), lroundf(_outputSize.cy * scale.y) });
		}

		// windowSinc ����
		it = props.find("windowSinc");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� windowSinc ����ֵ");

			float windowSinc = value.get<float>();
			Debug::Assert(
				windowSinc > 0,
				L"�Ƿ��� windowSinc ����ֵ"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(Jinc2ScaleEffect::PROP_WINDOW_SINC, windowSinc),
				L"���� windowSinc ����ʧ��"
			);
		}

		// sinc ����
		it = props.find("sinc");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� sinc ����ֵ");

			float sinc = value.get<float>();
			Debug::Assert(
				sinc > 0,
				L"�Ƿ��� sinc ����ֵ"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(Jinc2ScaleEffect::PROP_SINC, sinc),
				L"���� sinc ����ʧ��"
			);
		}

		// ARStrength ����
		it = props.find("ARStrength");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� ARStrength ����ֵ");

			float ARStrength = value.get<float>();
			Debug::Assert(
				ARStrength >= 0 && ARStrength <= 1,
				L"�Ƿ��� ARStrength ����ֵ"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(Jinc2ScaleEffect::PROP_AR_STRENGTH, ARStrength),
				L"���� ARStrength ����ʧ��"
			);
		}
		
		// �滻 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddMitchellNetravaliScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_MITCHELL_NETRAVALI_SCALE_EFFECT, 
			&MitchellNetravaliScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateEffect(CLSID_MAGPIE_MITCHELL_NETRAVALI_SCALE_EFFECT, &effect),
			L"���� Mitchell-Netraval Scale Effect ʧ��"
		);

		// scale ����
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);

			Debug::ThrowIfComFailed(
				effect->SetValue(MitchellNetravaliScaleEffect::PROP_SCALE, scale),
				L"���� scale ����ʧ��"
			);

			// ���� scale �����ͼ��ߴ�ı�
			_SetDestSize(SIZE{ lroundf(_outputSize.cx * scale.x), lroundf(_outputSize.cy * scale.y) });
		}

		// useSharperVersion ����
		it = props.find("useSharperVersion");
		if (it != props.end()) {
			const auto& val = *it;
			Debug::Assert(val.is_boolean(), L"�Ƿ��� useSharperVersion ����ֵ");

			Debug::ThrowIfComFailed(
				effect->SetValue(MitchellNetravaliScaleEffect::PROP_USE_SHARPER_VERSION, (BOOL)val.get<bool>()),
				L"���� useSharperVersion ����ʧ��"
			);
		}

		// �滻 output effect
		_PushAsOutputEffect(effect);
	}

	// ���õ� HIGH_QUALITY_CUBIC �����㷨����Сͼ��ʱЧ������
	void _AddHQBicubicScaleEffect(const nlohmann::json& props) {
		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateEffect(CLSID_D2D1Scale, &effect),
			L"���� Anime4K Effect ʧ��"
		);

		effect->SetValue(D2D1_SCALE_PROP_INTERPOLATION_MODE, D2D1_SCALE_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
		effect->SetValue(D2D1_SCALE_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);

		// scale ����
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);
			Debug::ThrowIfComFailed(
				effect->SetValue(D2D1_SCALE_PROP_SCALE, scale),
				L"���� scale ����ʧ��"
			);

			// ���� scale �����ͼ��ߴ�ı�
			_SetDestSize(SIZE{ lroundf(_outputSize.cx * scale.x), lroundf(_outputSize.cy * scale.y) });
		}

		// sharpness ����
		it = props.find("sharpness");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� sharpness ����ֵ");

			float sharpness = value.get<float>();
			Debug::Assert(
				sharpness >= 0 && sharpness <= 1,
				L"�Ƿ��� sharpness ����ֵ"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(D2D1_SCALE_PROP_SHARPNESS, sharpness),
				L"���� sharpness ����ʧ��"
			);
		}

		_PushAsOutputEffect(effect);
	}

	void _AddLanczos6ScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_LANCZOS6_SCALE_EFFECT,
			&Lanczos6ScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateEffect(CLSID_MAGPIE_LANCZOS6_SCALE_EFFECT, &effect),
			L"���� Lanczos6 Effect ʧ��"
		);

		// scale ����
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);

			Debug::ThrowIfComFailed(
				effect->SetValue(Lanczos6ScaleEffect::PROP_SCALE, scale),
				L"���� scale ����ʧ��"
			);

			// ���� scale �����ͼ��ߴ�ı�
			_SetDestSize(SIZE{ lroundf(_outputSize.cx * scale.x), lroundf(_outputSize.cy * scale.y) });
		}

		// ARStrength ����
		it = props.find("ARStrength");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� ARStrength ����ֵ");

			float ARStrength = value.get<float>();
			Debug::Assert(
				ARStrength >= 0 && ARStrength <= 1,
				L"�Ƿ��� ARStrength ����ֵ"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(Lanczos6ScaleEffect::PROP_AR_STRENGTH, ARStrength),
				L"���� ARStrengthc ����ʧ��"
			);
		}

		// �滻 output effect
		_PushAsOutputEffect(effect);
	}


	D2D1_VECTOR_2F _ReadScaleProp(const nlohmann::json& prop) {
		Debug::Assert(
			prop.is_array() && prop.size() == 2
			&& prop[0].is_number() && prop[1].is_number(),
			L"��ȡ scale ����ʧ��"
		);

		D2D1_VECTOR_2F scale{ prop[0], prop[1] };
		Debug::Assert(
			scale.x >= 0 && scale.y >= 0,
			L"scale ���Ե�ֵ�Ƿ�"
		);

		if (scale.x == 0 || scale.y == 0) {
			// ���ͼ�������Ļ
			scale.x = min(
				float(_hostClient.right - _hostClient.left) / _outputSize.cx,
				float(_hostClient.bottom - _hostClient.top) / _outputSize.cy
			);
			scale.y = scale.x;
		}

		return scale;
	}

	// �� effect ��ӵ� effect ����Ϊ���
	void _PushAsOutputEffect(ComPtr<ID2D1Effect> effect) {
		if (_firstEffect) {
			effect->SetInputEffect(0, _outputEffect.Get());
			_outputEffect = effect;
		} else {
			_outputEffect = _firstEffect = effect;
		}
	}

	
	void _SetDestSize(SIZE value) {
		// �ƺ�������Ҫ���� tile
		/*if (value.cx > _outputSize.cx || value.cy > _outputSize.cy) {
		    // ���� tile �Ĵ�С������ͼ��
			D2D1_RENDERING_CONTROLS rc{};
			_d2dContext.GetD2DDC()->GetRenderingControls(&rc);
			
			rc.tileSize.width = max(value.cx, _outputSize.cx);
			rc.tileSize.height = max(value.cy, _outputSize.cy);
			_d2dContext.GetD2DDC()->SetRenderingControls(rc);
		}*/

		_outputSize = value;
	}


	// ��Ҫʱע�� effect
	void _CheckAndRegisterEffect(const GUID& effectID, std::function<HRESULT(ID2D1Factory1*)> registerFunc) {
		if (_registeredEffects.find(effectID) == _registeredEffects.end()) {
			// δע��
			Debug::ThrowIfComFailed(
				registerFunc(_d2dContext.GetD2DFactory()),
				L"ע�� Effect ʧ��"
			);
			
			_registeredEffects.insert(effectID);
		}
	}

	ComPtr<ID2D1Bitmap> _inputBmp = nullptr;

	ComPtr<ID2D1Effect> _firstEffect = nullptr;
	ComPtr<ID2D1Effect> _outputEffect = nullptr;

	// ���ͼ��ߴ�
	SIZE _outputSize{};
	D2D1_RECT_F _outputRect{};
	
	const RECT& _hostClient;

	// �洢��ע��� effect �� GUID
	std::unordered_set<GUID> _registeredEffects;

	// tile �Ĵ�С
	SIZE _maxDestSize{};
};
