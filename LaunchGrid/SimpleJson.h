#pragma once


namespace simplejson
{
	enum ValueType
	{
		VT_NUMBER,
		VT_STRING,
		VT_ARRAY,
		VT_OBJECT,
		VT_UNDEFINED,
	};

	class ValueRef;
	class ValueIterator;

	class Value
	{
	public:
		typedef ValueIterator iterator;

		Value();
		static Value& undefined();
		static Value number(double number);
		static Value string(const wchar_t* str);
		static Value string(const std::wstring& str);
		static Value object();
		static Value array();

		ValueType type() const;

		operator bool() const;
		double asNumber(double defaultValue = 0.0) const;
		std::wstring asString(const wchar_t *defaultValue = L"") const;

		size_t length() const;
		ValueRef operator[](size_t index) const;
		void push(const Value& value);
		void remove(size_t index);

		ValueRef operator[](const wchar_t* key) const;
		ValueRef operator[](const std::wstring& key) const;
		std::vector<std::wstring> objectKeys() const;
		Value setDefault(const wchar_t* key, const Value& value);
		bool has(const wchar_t* key) const;

		iterator begin();
		iterator end();
	private:
		Value(ValueType t);
	protected:
		struct ValueImpl
		{
			ValueType m_type;
			double m_number;
			std::wstring m_string;
			std::vector<Value> m_array;
			std::map<std::wstring, Value> m_object;
		};

		std::shared_ptr<ValueImpl> m_impl;

		friend class ValueRef;
		friend class ValueIterator;
	};

	class ValueRef : public Value
	{
	public:
		ValueRef& operator=(const Value& other);
		ValueRef& operator=(const ValueRef& other);
		Value key() const;
	private:
		ValueRef();
		ValueRef(const std::shared_ptr<ValueImpl>& parent, size_t index);
		ValueRef(const std::shared_ptr<ValueImpl>& parent, const wchar_t* index);

		std::shared_ptr<ValueImpl> m_parent;
		size_t m_arrayIndex;
		std::wstring m_objectIndex;

		friend class Value;
	};

	class ValueIterator
	{
	public:
		ValueIterator& operator++()
		{
			if (m_parent.type() == VT_ARRAY)
			{
				++m_arrayIndex;
			}
			else
			{
				++m_objectIterator;
			}
			return *this;
		}
		ValueRef operator*() const
		{
			if (m_parent.type() == VT_ARRAY)
			{
				return m_parent[m_arrayIndex];
			}
			else
			{
				return m_parent[m_objectIterator->first];
			}
		}
		ValueRef operator->() const
		{
			if (m_parent.type() == VT_ARRAY)
			{
				return m_parent[m_arrayIndex];
			}
			else
			{
				return m_parent[m_objectIterator->first];
			}
		}
		bool operator==(const ValueIterator& other) const
		{
			if (m_parent.m_impl != other.m_parent.m_impl)
			{
				return false;
			}
			if (m_parent.type() == VT_ARRAY)
			{
				return m_arrayIndex == other.m_arrayIndex;
			}
			else
			{
				return m_objectIterator == other.m_objectIterator;
			}
		}
		bool operator!=(const ValueIterator& other) const
		{
			return !operator==(other);
		}
	private:
		Value m_parent;
		ValueType m_type;
		size_t m_arrayIndex;
		std::map<std::wstring, Value>::iterator m_objectIterator;

		friend class Value;
	};

	Value parse(const char* stream);
	std::string dump(const Value& value);
	std::wstring wdump(const Value& value);
};
