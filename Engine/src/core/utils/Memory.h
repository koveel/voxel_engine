#pragma once

// unique_ptr
template<typename T>
class owning_ptr
{
public:
	owning_ptr() = default;
	owning_ptr(const owning_ptr&) = delete;
	owning_ptr(owning_ptr&& other) noexcept
		: m_Ptr(other.m_Ptr)
	{
		other.m_Ptr = nullptr;
	}
	owning_ptr(T* raw)
		: m_Ptr(raw)
	{
	}
	~owning_ptr()
	{
		delete m_Ptr;
	}
	owning_ptr& operator=(const owning_ptr&) = delete;
	owning_ptr& operator=(owning_ptr&& other) noexcept
	{
		m_Ptr = other.m_Ptr;
		other.m_Ptr = nullptr;
		return *this;
	}

	T* get() { return m_Ptr; }
	T* get() const { return m_Ptr; }

	T& operator*() { return *m_Ptr; }
	T& operator*() const { return *m_Ptr; }

	T* operator->() { return m_Ptr; }
	T* operator->() const { return m_Ptr; }

	void reset(T* ptr = nullptr)
	{
		delete m_Ptr;
		m_Ptr = ptr;
	}

	operator bool() const { return m_Ptr; }
private:
	T* m_Ptr = nullptr;
};

template<typename T, typename... Args>
static owning_ptr<T> make_owning(Args&&... args)
{
	return owning_ptr<T>{ new T(std::forward<Args>(args)...) };
}

// shared_ptr
template<typename T>
class ref_counted
{
public:
	struct control_block_t
	{
		std::atomic<size_t> reference_count = 0;

		virtual ~control_block_t() = default;
		virtual void destroy_managed() {}

		void add_ref()
		{
			reference_count++;
		}

		void drop_ref()
		{
			reference_count--;
			if (reference_count != 0)
				return;

			// destroy/delete managed object and self
			destroy_managed();
			delete this;
		}
	};

	struct inplace_control_block : public control_block_t
	{
		T managed;

		template<typename... Args>
		inplace_control_block(Args&&... args)
			: managed(std::forward<Args>(args)...)
		{
		}
	};

	struct dynamic_control_block : public control_block_t
	{
		T* managed = nullptr;

		dynamic_control_block(T* managed)
			: managed(managed)
		{
			control_block_t::add_ref();
		}

		void destroy_managed() override
		{
			delete managed;
		}
	};
public:
	ref_counted() = default;
	ref_counted(control_block_t* control, T* ptr)
		: m_control(control), m_ptr(ptr)
	{
		m_control->add_ref();
	}
	ref_counted(T* ptr)
		: m_ptr(ptr)
	{
		m_control = new dynamic_control_block(ptr);
	}
	ref_counted(const ref_counted<T>& other)
		: m_control(other.m_control), m_ptr(other.m_ptr)
	{
		add_control_ref();
	}

	~ref_counted()
	{
		drop_control_ref();
	}

	ref_counted<T>& operator=(const ref_counted<T>& other)
	{
		m_ptr = other.m_ptr;
		m_control = other.m_control;
		add_control_ref();
		return *this;
	}

	T* get() const { return m_ptr; }

	void reset(T* ptr = nullptr)
	{
		drop_control_ref();

		if (!ptr)
		{
			m_control = nullptr;
			m_ptr = nullptr;
			return;
		}

		m_ptr = ptr;
		m_control = new dynamic_control_block(ptr);
	}

	bool empty() const { return m_ptr; }
	operator bool() const { return m_ptr; }

	T& operator*() { return *m_ptr; }
	const T& operator*() const { return *m_ptr; }

	T* operator->() { return m_ptr; }
	const T* operator->() const { return m_ptr; }
private:
	void add_control_ref()
	{
		if (m_control)
			m_control->add_ref();
	}
	void drop_control_ref()
	{
		if (m_control)
			m_control->drop_ref();
	}
private:
	T* m_ptr = nullptr;
	control_block_t* m_control = nullptr;
};

template<typename T, typename... Args>
static ref_counted<T> make_ref_counted(Args&&... args)
{
	auto control_block = new ref_counted<T>::inplace_control_block(std::forward<Args>(args)...);
	return &control_block->managed;
}