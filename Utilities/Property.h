#pragma once

// Property implementation
namespace prp
{
	// Default property base provider (contains base pointer)
	template<typename BT>
	class _base
	{
		BT* const m_ptr;

	public:
		_base(BT* ptr)
			: m_ptr(ptr)
		{
		}

		BT* get() const
		{
			return m_ptr;
		}
	};

	// Empty property base provider (can only be used with default setters/getters)
	struct _empty_base
	{
		_empty_base() = default;

		_empty_base(void*)
		{
		}

		std::nullptr_t get() const
		{
			return nullptr;
		}
	};

	// Default property value provider (contains value)
	template<typename BT, typename T>
	class _value
	{
		T m_value;

	public:
		_value() = default;

		template<typename... Args, typename = std::enable_if_t<std::is_constructible<T, Args...>::value>> _value(BT*, Args&&... args)
			: m_value(std::forward<Args>(args)...)
		{
		}

		T* get()
		{
			return &m_value;
		}

		const T* get() const
		{
			return &m_value;
		}

		friend BT;
	};

	// External property value provider
	template<typename BT, typename T, T BT::*Member>
	class _external_value
	{
		T* const m_ptr;

	public:
		_external_value(BT* base)
			: m_ptr(&(base->*Member))
		{
		}

		T* get()
		{
			return m_ptr;
		}

		const T* get() const
		{
			return m_ptr;
		}
	};

	// Empty property value provider
	struct _empty_value
	{
		_empty_value() = default;

		_empty_value(void*)
		{
		}

		std::nullptr_t get()
		{
			return nullptr;
		}
	};

	// Default initializer (do nothing)
	template<typename BT, typename T>
	void _init(BT*, T*)
	{
	}

	// Default setter (assignment)
	template<typename BT, typename T>
	void _set(BT*, T* data, const T& value)
	{
		*data = value;
	}

	// Default getter
	template<typename BT, typename T>
	T _get(BT*, T* data)
	{
		return *data;
	}
	
	// Property function conversion helper (for Init, Set, Get property template arguments)
	template<typename FT, FT Func>
	struct _func;

	template<typename BT, typename T, void(*Func)(BT&, T&)>
	struct _func<void(*)(BT&, T&), Func>
	{
		static void call(BT* base, T* data)
		{
			Func(*base, *data);
		}
	};

	template<typename BT, typename T, void(BT::*Method)(T&)>
	struct _func<void(BT::*)(T&), Method>
	{
		static void call(BT* base, T* data)
		{
			(base->*Method)(*data);
		}
	};

	template<typename BT, typename T, void(*Func)(BT&, T&, const T&)>
	struct _func<void(*)(BT&, T&, const T&), Func>
	{
		static void call(BT* base, T* data, const T& value)
		{
			Func(*base, *data, value);
		}
	};

	template<typename BT, typename T, void(BT::*Method)(T&, const T&)>
	struct _func<void(BT::*)(T&, const T&), Method>
	{
		static void call(BT* base, T* data, const T& value)
		{
			(base->*Method)(*data, value);
		}
	};

	template<typename BT, typename T, T(*Func)(BT&, T&)>
	struct _func<T(*)(BT&, T&), Func>
	{
		static T call(BT* base, T* data)
		{
			return Func(*base, *data);
		}
	};

	template<typename BT, typename T, T(BT::*Method)(T&)>
	struct _func<T(BT::*)(T&), Method>
	{
		static T call(BT* base, T* data)
		{
			return (base->*Method)(*data);
		}
	};

	// Property class (read/write property)
	// BT: base class which contains the property
	// T: property value type
	// BS: base pointer provider (BS::get())
	// VS: value storage provider (VS::get())
	// Init: initializer (nullptr - nothing)
	// Set: setter (nullptr - default setter)
	// Get: getter (nullptr - default getter)
	template
	<
		typename BT,
		typename T,
		typename BS = _base<BT>,
		typename VS = _value<BT, T>,
		void(*Init)(BT*, T*) = _init,
		void(*Set)(BT*, T*, const T&) = _set,
		T(*Get)(BT*, T*) = _get
	>
	class prop : public BS, public VS
	{
		using base = BS;
		using value = VS;

	public:
		prop() = default;

		template<typename... Args> prop(BT* base_ptr, Args&&... args)
			: base(base_ptr)
			, value(base_ptr, std::forward<Args>(args)...)
		{
			static_assert(Init, "This property cannot be initialized");
			Init(base::get(), value::get());
		}

		prop& operator =(const T& value)
		{
			static_assert(Set, "This property cannot be written");
			Set(base::get(), value::get(), value);
			return *this;
		}

		operator T() const
		{
			static_assert(Get, "This property cannot be read");
			return Get(base::get(), value::get());
		}
	};
}

// Convert to appropriate function type
#define PROP_FN(func) prp::_func<decltype(&func), func>::call
