#include "pch.h"
#include "HotkeySettings.h"
#if __has_include("HotkeySettings.g.cpp")
#include "HotkeySettings.g.cpp"
#endif

#include "Win32Utils.h"


namespace winrt::Magpie::App::implementation {

IVector<IInspectable> HotkeySettings::GetKeyList() const {
	std::vector<IInspectable> shortcutList;
	if (_win) {
		// Windows 键
		shortcutList.push_back(box_value(92));
	}

	if (_ctrl) {
		shortcutList.push_back(box_value(L"Ctrl"));
	}

	if (_alt) {
		shortcutList.push_back(box_value(L"Alt"));
	}

	if (_shift) {
		shortcutList.push_back(box_value(L"Shift"));
	}

	if (_code > 0) {
		switch (_code) {
		   // https://docs.microsoft.com/en-us/uwp/api/windows.system.virtualkey?view=winrt-20348
		case 38: // The Up Arrow key or button.
		case 40: // The Down Arrow key or button.
		case 37: // The Left Arrow key or button.
		case 39: // The Right Arrow key or button.
		// case 8: // The Back key or button.
		// case 13: // The Enter key or button.
			shortcutList.push_back(box_value(_code));
			break;
		default:
			std::wstring localKey = Win32Utils::GetKeyName(_code);
			shortcutList.push_back(box_value(localKey));
			break;
		}
	}

	return single_threaded_vector(std::move(shortcutList));
}

bool HotkeySettings::Check() const {
	return false;
}

}
