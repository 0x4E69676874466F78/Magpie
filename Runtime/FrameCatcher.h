#pragma once
#include "pch.h"
#include "Renderable.h"
#include <chrono>
using namespace std::chrono;


// ����֡��
class FrameCatcher : public Renderable {
public:
	FrameCatcher(const RECT& destRect) : _destRect(destRect) {
		Debug::ThrowIfComFailed(
			Env::$instance->GetDWFactory()->CreateTextFormat(
				L"Microsoft YaHei",
				nullptr,
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				20,
				L"en-us",
				&_dwTxtFmt
			),
			L"����IDWriteTextFormatʧ��"
		);

		Debug::ThrowIfComFailed(
			Env::$instance->GetD2DDC()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_d2dFPSTxtBrush),
			L"���� _d2dFPSTxtBrush ʧ��"
		);
	}

	void Render() override {
		_ReportNewFrame();

		// �����ı�
		std::wstring fps = boost::str(boost::wformat(L"%d FPS") % lround(_fps));
		Env::$instance->GetD2DDC()->DrawTextW(
			fps.c_str(),
			(UINT32)fps.size(),
			_dwTxtFmt.Get(),
			D2D1::RectF(
				FLOAT(_destRect.left + 10),
				FLOAT(_destRect.top + 10),
				FLOAT(_destRect.right),
				FLOAT(_destRect.bottom)
			),
			_d2dFPSTxtBrush.Get()
		);
		//Debug::WriteLine(_fps);
		//Debug::WriteLine(L"AVG: "s + std::to_wstring(_GetAvgFPS(cur)));
	}

private:
	void _ReportNewFrame() {
		if (_begin == steady_clock::time_point()) {
			// ��һ֡
			_lastBegin = _last = _begin = steady_clock::now();
			return;
		}

		++_allFrameCount;

		auto cur = steady_clock::now();
		auto ms = duration_cast<milliseconds>(cur - _lastBegin).count();
		if (ms < 1000) {
			++_frameCount;
			_last = cur;
			return;
		}

		// �ѹ�һ��
		_fps = _frameCount + double(duration_cast<milliseconds>(cur - _last).count() + 1000 - ms) / 1000;
		_frameCount += 1 - _fps;
		_lastBegin = cur;
	}

	double _GetAvgFPS(steady_clock::time_point cur) {
		auto ms = duration_cast<milliseconds>(cur - _begin).count();
		return static_cast<double>(_allFrameCount) * 1000 / ms;
	}

	// ���ڼ���ƽ��֡��
	steady_clock::time_point _begin;	// ��һ֡��ʱ��
	int _allFrameCount = 0;	// ����Ⱦ����֡��


	steady_clock::time_point _lastBegin;	// ����һ��Ŀ�ʼ
	steady_clock::time_point _last;			// ������һ֡��ʱ��
	double _frameCount = 0;					// ����һ��������Ⱦ��֡������һ��������
	
	double _fps = 0;

	ComPtr<IDWriteTextFormat> _dwTxtFmt = nullptr;
	ComPtr<ID2D1SolidColorBrush> _d2dFPSTxtBrush = nullptr;

	RECT _destRect;
};
