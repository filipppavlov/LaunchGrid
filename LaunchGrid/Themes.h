#pragma once

namespace theme
{
	enum Element
	{
		BACKGROUND,
		WINDOW_TITLE,
		TEXT,
		LINE,
	};

	COLORREF color(Element);
	HBRUSH brush(Element);
	HPEN pen(Element);
}