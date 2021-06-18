#pragma once
#include "pch.h"
#include "D2DContext.h"
#include "AdaptiveSharpenEffect.h"
#include "Anime4KEffect.h"
#include "Anime4KDarkLinesEffect.h"
#include "Anime4KThinLinesEffect.h"
#include "JincScaleEffect.h"
#include "MitchellNetravaliScaleEffect.h"
#include "LanczosScaleEffect.h"
#include "PixelScaleEffect.h"
#include "ACNetEffect.h"
#include "Anime4KDenoiseBilateralEffect.h"
#include "RavuLiteEffect.h"
#include "RavuZoomEffect.h"
#include "nlohmann/json.hpp"
#include <unordered_set>
#include "Env.h"


// ȡ���ڲ�ͬ�Ĳ���ʽ�����в�ͬ��������룬�����������ͨ�õĲ���
// �̳д�����Ҫʵ�� _PushAsOutputEffect��Apply
// ���ڹ��캯���е��� _Init
class EffectRendererBase {
public:
	EffectRendererBase() :
		_d2dDC(Env::$instance->GetD2DDC()),
		_d2dFactory(Env::$instance->GetD2DFactory())
	{
	}

	virtual ~EffectRendererBase() {}

	// ���ɸ��ƣ������ƶ�
	EffectRendererBase(const EffectRendererBase&) = delete;
	EffectRendererBase(EffectRendererBase&&) = delete;

	virtual ComPtr<ID2D1Image> Apply(IUnknown* inputImg) = 0;

protected:
	void _Init() {
		_ReadEffectsJson(Env::$instance->GetScaleModel());

		const RECT hostClient = Env::$instance->GetHostClient();
		const RECT srcClient = Env::$instance->GetSrcClient();

		float width = (srcClient.right - srcClient.left) * _scale.first;
		float height = (srcClient.bottom - srcClient.top) * _scale.second;
		float left = roundf((hostClient.right - hostClient.left - width) / 2);
		float top = roundf((hostClient.bottom - hostClient.top - height) / 2);
		Env::$instance->SetDestRect({left, top, left + width, top + height});
	}

	// �� effect ��ӵ� effect ����Ϊ���
	virtual void _PushAsOutputEffect(ComPtr<ID2D1Effect> effect) = 0;

private:
	void _ReadEffectsJson(const std::string_view& scaleModel) {
		const auto& models = nlohmann::json::parse(scaleModel);
		Debug::Assert(models.is_array(), L"json ��ʽ����");

		for (const auto &model : models) {
			Debug::Assert(model.is_object(), L"json ��ʽ����");

			const auto &effectType = model.value("effect", "");
			const auto& subType = model.value("type", "");

			if (effectType == "scale") {
				if (subType == "Anime4K") {
					_AddAnime4KEffect(model);
				} else if (subType == "ACNet") {
					_AddACNetEffect();
				} else if (subType == "jinc") {
					_AddJincScaleEffect(model);
				} else if (subType == "mitchell") {
					_AddMitchellNetravaliScaleEffect(model);
				} else if (subType == "HQBicubic") {
					_AddHQBicubicScaleEffect(model);
				} else if (subType == "lanczos") {
					_AddLanczosScaleEffect(model);
				} else if (subType == "pixel") {
					_AddPixelScaleEffect(model);
				} else if (subType == "ravuLite") {
					_AddRavuLiteEffect(model);
				} else if (subType == "ravuZoom") {
					_AddRavuZoomEffect(model);
				} else {
					Debug::Assert(false, L"δ֪�� scale effect");
				}
			} else if (effectType == "sharpen") {
				if (subType == "adaptive") {
					_AddAdaptiveSharpenEffect(model);
				} else if (subType == "builtIn") {
					_AddBuiltInSharpenEffect(model);
				} else {
					Debug::Assert(false, L"δ֪�� sharpen effect");
				}
			} else if (effectType == "misc") {
				if (subType == "Anime4KDarkLines") {
					_AddAnime4KDarkLinesEffect(model);
				} else if (subType == "Anime4KThinLines") {
					_AddAnime4KThinLinesEffect(model);
				} else if (subType == "Anime4KDenoiseBilateral") {
					_AddAnime4KDenoiseBilateral(model);
				} else {
					Debug::Assert(false, L"δ֪�� misc effect");
				}
			} else {
				Debug::Assert(false, L"δ֪�� effect");
			}
		}
	}

	void _AddRavuZoomEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_RAVU_ZOOM_EFFECT,
			&RavuZoomEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_RAVU_ZOOM_EFFECT, &effect),
			L"���� ravu zoom effect ʧ��"
		);

		// scale ����
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);

			Debug::ThrowIfComFailed(
				effect->SetValue(RavuZoomEffect::PROP_SCALE, scale),
				L"���� scale ����ʧ��"
			);

			// ���� scale �����ͼ��ߴ�ı�
			_scale.first *= scale.x;
			_scale.second *= scale.y;
		}

		// ����Ȩ������
		effect->SetInput(1, 
			Utils::LoadBitmapFromFile(Env::$instance->GetWICImageFactory(), _d2dDC, L"RavuZoomR3Weights.png").Get());

		// �滻 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddRavuLiteEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_RAVU_LITE_EFFECT,
			&RavuLiteEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_RAVU_LITE_EFFECT, &effect),
			L"���� ravu effect ʧ��"
		);

		// ���ͼ��ĳ��Ϳ��Ϊ 2 ��
		_scale.first *= 2;
		_scale.second *= 2;

		// �滻 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddAnime4KDenoiseBilateral(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ANIME4K_DENOISE_BILATERAL_EFFECT,
			&Anime4KDenoiseBilateralEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_DENOISE_BILATERAL_EFFECT, &effect),
			L"���� Anime4K denoise bilateral effect ʧ��"
		);

		// variant ����
		auto it = props.find("variant");
		if (it != props.end()) {
			Debug::Assert(it->is_string(), L"�Ƿ���variant����ֵ");
			std::string_view variant = *it;
			int v = 0;
			if (variant == "mode") {
				v = 0;
			} else if (variant == "median") {
				v = 1;
			} else if (variant == "mean") {
				v = 2;
			} else {
				Debug::Assert(false, L"�Ƿ���variant����ֵ");
			}
			
			Debug::ThrowIfComFailed(
				effect->SetValue(Anime4KDenoiseBilateralEffect::PROP_VARIANT, v),
				L"���� scale ����ʧ��"
			);
		}

		// intensity ����
		it = props.find("intensity");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ���intensity����ֵ");

			float intensity = value.get<float>();
			Debug::Assert(
				intensity > 0,
				L"�Ƿ���intensity����ֵ"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(Anime4KDenoiseBilateralEffect::PROP_INTENSITY, intensity),
				L"����intensity����ʧ��"
			);
		}

		// �滻 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddAdaptiveSharpenEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT,
			&AdaptiveSharpenEffect::Register
		);

		ComPtr<ID2D1Effect> adaptiveSharpenEffect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT, &adaptiveSharpenEffect),
			L"���� Adaptive sharpen effect ʧ��"
		);

		// curveHeight ����
		auto it = props.find("curveHeight");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� curveHeight ����ֵ");

			float curveHeight = value.get<float>();
			Debug::Assert(
				curveHeight > 0,
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
			_d2dDC->CreateEffect(CLSID_D2D1Sharpen, &d2dSharpenEffect),
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

	void _AddACNetEffect() {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ACNET_EFFECT,
			&ACNetEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ACNET_EFFECT, &effect),
			L"���� ACNet Effect ʧ��"
		);

		// ���ͼ��ĳ��Ϳ��Ϊ 2 ��
		_scale.first *= 2;
		_scale.second *= 2;

		_PushAsOutputEffect(effect);
	}

	void _AddAnime4KEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ANIME4K_EFFECT,
			&Anime4KEffect::Register
		);

		ComPtr<ID2D1Effect> anime4KEffect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_EFFECT, &anime4KEffect),
			L"���� Anime4K Effect ʧ��"
		);

		// curveHeight ����
		auto it = props.find("curveHeight");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� curveHeight ����ֵ");

			float curveHeight = value.get<float>();
			Debug::Assert(
				curveHeight >= 0,
				L"�Ƿ��� curveHeight ����ֵ"
			);

			Debug::ThrowIfComFailed(
				anime4KEffect->SetValue(Anime4KEffect::PROP_CURVE_HEIGHT, curveHeight),
				L"���� curveHeight ����ʧ��"
			);
		}

		// useDenoiseVersion ����
		it = props.find("useDenoiseVersion");
		if (it != props.end()) {
			const auto& val = *it;
			Debug::Assert(val.is_boolean(), L"�Ƿ��� useSharperVersion ����ֵ");

			Debug::ThrowIfComFailed(
				anime4KEffect->SetValue(Anime4KEffect::PROP_USE_DENOISE_VERSION, (BOOL)val.get<bool>()),
				L"���� useSharperVersion ����ʧ��"
			);
		}

		// ���ͼ��ĳ��Ϳ��Ϊ 2 ��
		_scale.first *= 2;
		_scale.second *= 2;

		_PushAsOutputEffect(anime4KEffect);
	}

	void _AddAnime4KDarkLinesEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ANIME4K_DARKLINES_EFFECT,
			&Anime4KDarkLinesEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_DARKLINES_EFFECT, &effect),
			L"���� Anime4K Effect ʧ��"
		);

		// strength ����
		auto it = props.find("strength");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� strength ����ֵ");

			float strength = value.get<float>();
			Debug::Assert(
				strength > 0,
				L"�Ƿ��� strength ����ֵ"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(Anime4KDarkLinesEffect::PROP_STRENGTH, strength),
				L"���� strength ����ʧ��"
			);
		}

		_PushAsOutputEffect(effect);
	}

	void _AddAnime4KThinLinesEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ANIME4K_THINLINES_EFFECT,
			&Anime4KThinLinesEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_THINLINES_EFFECT, &effect),
			L"���� Anime4K Effect ʧ��"
		);

		// strength ����
		auto it = props.find("strength");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"�Ƿ��� strength ����ֵ");

			float strength = value.get<float>();
			Debug::Assert(
				strength > 0,
				L"�Ƿ��� strength ����ֵ"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(Anime4KThinLinesEffect::PROP_STRENGTH, strength),
				L"���� strength ����ʧ��"
			);
		}

		_PushAsOutputEffect(effect);
	}

	
	void _AddJincScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_JINC_SCALE_EFFECT,
			&JincScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_JINC_SCALE_EFFECT, &effect),
			L"���� Jinc Effect ʧ��"
		);
		

		// scale ����
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);
			
			Debug::ThrowIfComFailed(
				effect->SetValue(JincScaleEffect::PROP_SCALE, scale),
				L"���� scale ����ʧ��"
			);

			// ���� scale �����ͼ��ߴ�ı�
			_scale.first *= scale.x;
			_scale.second *= scale.y;
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
				effect->SetValue(JincScaleEffect::PROP_WINDOW_SINC, windowSinc),
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
				effect->SetValue(JincScaleEffect::PROP_SINC, sinc),
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
				effect->SetValue(JincScaleEffect::PROP_AR_STRENGTH, ARStrength),
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
			_d2dDC->CreateEffect(CLSID_MAGPIE_MITCHELL_NETRAVALI_SCALE_EFFECT, &effect),
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
			_scale.first *= scale.x;
			_scale.second *= scale.y;
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

	// ���õ� HIGH_QUALITY_CUBIC �����㷨
	void _AddHQBicubicScaleEffect(const nlohmann::json& props) {
		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_D2D1Scale, &effect),
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
			_scale.first *= scale.x;
			_scale.second *= scale.y;
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

	void _AddLanczosScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_LANCZOS_SCALE_EFFECT,
			&LanczosScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_LANCZOS_SCALE_EFFECT, &effect),
			L"���� Lanczos Effect ʧ��"
		);

		// scale ����
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);

			Debug::ThrowIfComFailed(
				effect->SetValue(LanczosScaleEffect::PROP_SCALE, scale),
				L"���� scale ����ʧ��"
			);

			// ���� scale �����ͼ��ߴ�ı�
			_scale.first *= scale.x;
			_scale.second *= scale.y;
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
				effect->SetValue(LanczosScaleEffect::PROP_AR_STRENGTH, ARStrength),
				L"���� ARStrengthc ����ʧ��"
			);
		}

		// �滻 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddPixelScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_PIXEL_SCALE_EFFECT,
			&PixelScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_PIXEL_SCALE_EFFECT, &effect),
			L"���� Pixel Scale Effect ʧ��"
		);

		// scale ����
		auto it = props.find("scale");
		if (it != props.end()) {
			Debug::Assert(it->is_number_integer(), L"�Ƿ���Scale����ֵ");
			int scale = *it;

			Debug::Assert(scale > 0, L"�Ƿ���Scale����ֵ");
			Debug::ThrowIfComFailed(
				effect->SetValue(PixelScaleEffect::PROP_SCALE, scale),
				L"���� scale ����ʧ��"
			);

			_scale.first *= scale;
			_scale.second *= scale;
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
			SIZE hostSize = Utils::GetSize(Env::$instance->GetHostClient());
			SIZE srcSize = Utils::GetSize(Env::$instance->GetSrcClient());

			// ���ͼ�������Ļ
			float x = float(hostSize.cx) / srcSize.cx / _scale.first;
			float y = float(hostSize.cy) / srcSize.cy / _scale.second;

			scale.x = min(x, y);
			scale.y = scale.x;
		}

		return scale;
	}
	

	// ��Ҫʱע�� effect
	void _CheckAndRegisterEffect(const GUID& effectID, std::function<HRESULT(ID2D1Factory1*)> registerFunc) {
		if (_registeredEffects.find(effectID) == _registeredEffects.end()) {
			// δע��
			Debug::ThrowIfComFailed(
				registerFunc(_d2dFactory),
				L"ע�� Effect ʧ��"
			);
			
			_registeredEffects.insert(effectID);
		}
	}

private:
	// ���ͼ��ߴ�
	std::pair<float, float> _scale{ 1.0f,1.0f };

	// �洢��ע��� effect �� GUID
	std::unordered_set<GUID> _registeredEffects;

	ID2D1Factory1* _d2dFactory;
	ID2D1DeviceContext* _d2dDC;
};
