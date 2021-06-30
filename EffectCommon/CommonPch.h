#pragma once


#define WIN32_LEAN_AND_MEAN             // �� Windows ͷ�ļ����ų�����ʹ�õ�����
// Windows ͷ�ļ�
#include <windows.h>
#include <wrl.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>

// C++ ����ʱͷ�ļ�
#include <string>
#include <cassert>

#pragma comment(lib, "d2d1.lib")

#include "CommonDebug.h"
#include "EffectUtils.h"

#define XML(X) TEXT(#X)

#define API_DECLSPEC extern "C" __declspec(dllexport)

using namespace Microsoft::WRL;
using namespace std::literals::string_literals;

