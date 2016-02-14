#pragma once

void enableLogging(bool enable);
void log(const wchar_t* format, ...);

#define LOG(format, ...) log(format, ##__VA_ARGS__)