#pragma once
#include "pch.h"
#include "SimpleScaleTransform.h"
#include "GUIDs.h"


// Ϊ Jinc2ScaleShader.hlsl �ṩ����
// ������
//   windowSinc���������0��ֵԽСͼ��Խ�����������о�ݡ�Ĭ��ֵΪ 0.5
//   sinc���������0��ֵԽ������Խ�����������ж�����Ĭ��ֵΪ 0.825
//   ARStrength��������ǿ�ȡ������� 0~1 ֮�䡣Ĭ��ֵΪ 0.5
class JincScaleTransform : public SimpleScaleTransform {
private:
    JincScaleTransform() : SimpleScaleTransform(GUID_MAGPIE_JINC2_SCALE_SHADER) {}
public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ JincScaleTransform** ppOutput) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = LoadShader(d2dEC, MAGPIE_JINC2_SCALE_SHADER, GUID_MAGPIE_JINC2_SCALE_SHADER);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new JincScaleTransform();
        return hr;
    }

    void SetWindowSinc(float value) {
        assert(value > 0);
        _windowSinc = value;
    }

    float GetWindowSinc() const {
        return _windowSinc;
    }

    void SetSinc(float value) {
        assert(value > 0);
        _sinc = value;
    }

    float GetSinc() const {
        return _sinc;
    }

    void SetARStrength(float value) {
        assert(value >= 0 && value <= 1);
        _ARStrength = value;
    }

    float GetARStrength() {
        return _ARStrength;
    }

protected:
    void _SetShaderContantBuffer(const SIZE& srcSize, const SIZE& destSize) override {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            INT32 destWidth;
            INT32 destHeight;
            float windowSinc;
            float sinc;
            float arStrength;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy,
            destSize.cx,
            destSize.cy,
            _windowSinc,
            _sinc,
            _ARStrength
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }
private:
    float _windowSinc = 0.5f;
    float _sinc = 0.825f;
    float _ARStrength = 0.5f;
};
