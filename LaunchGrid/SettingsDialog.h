#pragma once

enum SettingsPage
{
	SETTINGS_GENERAL,
	SETTINGS_CONTENT,
	SETTINGS_MENU,
};

bool ShowSettingsDialog(HWND parent, SettingsPage defaultPage = SETTINGS_GENERAL);