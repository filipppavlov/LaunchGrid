#include "stdafx.h"
#include "Themes.h"
#include "Settings.h"

namespace
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

	HBRUSH s_brushes[2][3] = { nullptr };
	HPEN s_pens[2][3] = { nullptr };

	int themeIndex()
	{
		return std::max(std::min(int(settings::settings()[L"general"][L"theme"].asNumber()), 1), 0);
	}
}

namespace theme
{
	COLORREF color(Element element)
	{
		return (themeIndex() ? s_dark : s_light)[element];
	}

	HBRUSH brush(Element element)
	{
		auto theme = themeIndex();
		if (!s_brushes[theme][element])
		{
			s_brushes[theme][element] = CreateSolidBrush(color(element));
		}
		return s_brushes[theme][element];
	}

	HPEN pen(Element element)
	{
		auto theme = themeIndex();
		if (!s_pens[theme][element])
		{
			s_pens[theme][element] = CreatePen(PS_SOLID, 0, color(element));
		}
		return s_pens[theme][element];
	}
}