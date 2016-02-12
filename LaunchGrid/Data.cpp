#include "stdafx.h"
#include "Data.h"
#include <Shlwapi.h>

namespace
{

	void split(const std::wstring& separator, const std::wstring& string, std::vector<std::wstring>& result)
	{
		result.clear();
		size_t offset = 0;
		size_t found;
		while ((found = string.find(separator, offset)) != std::wstring::npos)
		{
			result.push_back(string.substr(offset, found - offset));
			offset = found + separator.length();
		}
		result.push_back(string.substr(offset));
	}

	bool transformValue(const std::vector<std::wstring>& components, std::wstring& result, size_t offset = 0)
	{
		std::wstring subst;
		if (offset + 1 == components.size())
		{
			result = components[offset];
			return true;
		}

		if (components[offset] == L"FileName")
		{
			if (!transformValue(components, result, offset + 1))
			{
				return false;
			}
			auto v = result.c_str();
			auto fn = PathFindFileName(v);
			auto ext = PathFindExtension(v);
			result = result.substr(fn - v, ext - fn);
			return true;
		}
		else if (components[offset] == L"Directory")
		{
			if (!transformValue(components, result, offset + 1))
			{
				return false;
			}
			auto fn = PathFindFileName(result.c_str());
			result = result.substr(0, fn - result.c_str());
			return true;
		}
		else if (components.size() == 2 && components[0] == L"Extension")
		{
			if (!transformValue(components, result, offset + 1))
			{
				return false;
			}
			auto ext = PathFindExtension(result.c_str());
			result = result.substr(ext - result.c_str());
			return true;
		}
		else if (components.size() == 4 && components[0] == L"Path")
		{
			if (!transformValue(components, result, offset + 3))
			{
				return false;
			}
			wchar_t* stop;
			auto begin = std::wcstol(components[1].c_str(), &stop, 10);
			if (*stop)
			{
				return false;
			}
			auto end = std::wcstol(components[2].c_str(), &stop, 10);
			if (*stop)
			{
				return false;
			}

			std::vector<std::wstring> paths;
			const wchar_t* p = result.c_str();
			const wchar_t* next = p;
			while (*next)
			{
				auto from = next;
				next = PathFindNextComponent(next);
				paths.push_back(std::wstring(from, *next ? next - 1 : next));
			}

			if (begin < 0)
			{
				begin = int(paths.size()) + begin;
			}
			if (end < 0)
			{
				end = int(paths.size()) + end;
			}
			if (begin >= int(paths.size()))
			{
				begin = paths.size() - 1;
			}
			if (end >= int(paths.size()))
			{
				end = paths.size() - 1;
			}
			if (end < begin)
			{
				end = begin;
			}
			result = L"";
			for (int i = begin; i <= end; ++i)
			{
				if (i > begin)
				{
					result += L"\\";
				}
				result += paths[i];
			}
			return true;
		}
		else
		{
			return false;
		}
	}
}

BaseOption* BaseOption::create(simplejson::Value value)
{
	BaseOption* option;
	auto type = value[L"type"].asString();
	if (type == L"dir")
	{
		option = new DirectoryOption();
	}
	else if (type == L"file")
	{
		option = new FileOption();
	}
	else if (type == L"app")
	{
		option = new AppOption();
	}
	else
	{
		option = new TextOption();
	}
	option->load(value);
	return option;
}

void BaseOption::load(simplejson::Value value)
{
	m_name = value[L"name"].asString();
	m_position = OptionPosition(int(value[L"position"].asNumber()));
}

OptionPosition BaseOption::position() const
{
	return m_position;
}

const wchar_t * BaseOption::displayName() const
{
	return m_name.c_str();
}

std::wstring BaseOption::expandValue(size_t index, const std::map<BaseOption*, size_t>& values)
{
	return expandString(value(index).value, values);
}

std::wstring BaseOption::expandString(const std::wstring& string, const std::map<BaseOption*, size_t>& values)
{
	std::map<std::wstring, std::wstring> substitutions;
	for (auto i : values)
	{
		substitutions[i.first->displayName()] = i.first->value(i.second).value;
	}
	return expandString(string, substitutions);
}

std::wstring BaseOption::expandString(const std::wstring& string, const std::map<std::wstring, std::wstring>& substitutions)
{
	static std::wregex expr(L"\\$\\(([^\\)]+)\\)");
	std::wstring result = string;
	std::wsmatch match;
	bool matchFound = true;
	auto start = cbegin(result);
	while (std::regex_search(start, cend(result), match, expr))
	{
		std::vector<std::wstring> components;
		split(L":", match[1].str(), components);
		auto found = substitutions.find(components.back());
		if (found != substitutions.end())
		{
			components.pop_back();
			components.push_back(found->second);

			std::wstring subst;
			if (transformValue(components, subst))
			{
				result = result.substr(0, (start - result.begin()) + match.prefix().length()) + subst + result.substr(match.suffix().first - result.begin());
				start = result.begin() + match.prefix().length();
			}
			else
			{
				start = match.suffix().first;
			}
		}
		else
		{
			start = match.suffix().first;
		}
	}
	return result;
}


TextOption::TextOption()
{
}

void TextOption::load(simplejson::Value value)
{
	BaseOption::load(value);
	for (auto e : value[L"values"])
	{
		Value v;
		v.name = e[L"name"].asString();
		v.value = e[L"value"].asString();
		m_values.push_back(v);
	}
}

size_t TextOption::valueCount() const
{
	return m_values.size();
}

TextOption::Value TextOption::value(size_t index) const
{
	return m_values[index];
}

std::wstring TextOption::defaultValue() const
{
	return m_defaultValue;
}


void AppOption::load(simplejson::Value value)
{
	TextOption::load(value);

	for (auto e : value[L"values"])
	{
		AppOptions opt;
		opt.arguments = e[L"args"].asString();
		opt.verb = e[L"verb"].asString();
		opt.runHidden = e[L"hidden"].asNumber() != 0;
		m_options.push_back(opt);
	}
}

std::wstring AppOption::arguments(size_t index, const std::map<BaseOption*, size_t>& values) const
{
	return expandString(m_options[index].arguments, values);
}

bool AppOption::runHidden(size_t index) const
{
	return m_options[index].runHidden;
}

std::wstring AppOption::verb(size_t index) const
{
	return m_options[index].verb;
}


size_t BaseDirectoryOption::valueCount() const
{
	return m_values.size();
}

BaseOption::Value BaseDirectoryOption::value(size_t index) const
{
	return m_values[index];
}

std::wstring BaseDirectoryOption::defaultValue() const
{
	return L"";
}

size_t BaseDirectoryOption::patternCount() const
{
	return m_patterns.size();
}

const std::wstring & BaseDirectoryOption::pattern(size_t index) const
{
	return m_patterns[index].value;
}

void BaseDirectoryOption::load(simplejson::Value value)
{
	BaseOption::load(value);
	for (auto e : value[L"values"])
	{
		Value v;
		v.name = e[L"name"].asString();
		v.value = e[L"value"].asString();
		m_patterns.push_back(v);
	}
}

BaseDirectoryOption::BaseDirectoryOption(bool directory)
	:m_directory(directory)
{
}

void BaseDirectoryOption::prepareValues()
{
	for (auto& pattern: m_patterns)
	{
		::findFiles(pattern.name, displayName(), pattern.value, m_directory, [&](const std::wstring& name, const std::wstring& path) {
			Value value;
			value.name = name;
			value.value = path;
			m_values.push_back(value);
		});
	}
}

void splitPath(const wchar_t * path, std::vector<std::wstring>& components)
{
	auto start = path;
	auto c = path;
	do
	{
		switch (*c)
		{
		case L'\\':
		case L'/':
			components.push_back(std::wstring(start, c - start) + L"\0");
			components.push_back(std::wstring(1, *c) + L"\0");
			start = c + 1;
			break;
		}
		++c;
	} while (*c);
	components.push_back(std::wstring(start, c - start) + L"\0");
}
