#include "stdafx.h"
#include "Themes.h"
#include "Settings.h"

namespace theme
{

	COLORREF s_light[] = {
		RGB(255, 255, 255),  // BACKGROUND
		RGB(184, 184, 184),  // WINDOW_TITLE
		RGB(97, 97, 97),     // TEXT
		RGB(219, 219, 219),  // LINE
	};

	COLORREF s_dark[] = {
		RGB(30, 30, 30),     // BACKGROUND
		RGB(174, 174, 174),  // WINDOW_TITLE
		RGB(215, 215, 215),  // TEXT
		RGB(88, 88, 88),     // LINE
	};

	COLORREF color(Element element)
	{
		int theme = int(settings::settings()[L"general"][L"theme"].asNumber());
		return (theme ? s_dark : s_light)[element];
	}
}