#pragma once

#include "SimpleJson.h"

namespace settings
{ 
	std::wstring settingsDirectory();

	simplejson::Value settings();
	void setSettings(simplejson::Value);

	simplejson::Value cache();
	void setCache(simplejson::Value);
	void saveCache();
}