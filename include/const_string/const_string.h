#ifndef CONST_STRING_CONST_STRING_H
#define CONST_STRING_CONST_STRING_H

#include <string_view>
#include <algorithm>
#include <atomic>
#include <memory>

class const_string : public std::string_view {
public:
	/* #################### CTORS ########################## */
	//Default ConstString points at empty string
	const_string() :
		std::string_view()
	{};

	const_string(std::string_view other)
	{
		_copyFrom(other);
	}

	//NOTE: Use only for string literals (arrays with static storage duration)!!!
	template<size_t N>
	constexpr const_string(const char(&other)[N]) noexcept :
	std::string_view(other)
		//we don't have to initialize the shared_ptr to anything as string litterals already have static lifetime
	{
		static_assert(N >= 1, "");
	}

	// don't accept c-strings in the form of pointer
	// if you need to create a const_string from a c string use the <const_string(const char*, size_t)> constructor
	// if you need to create a ConstString form a string literal, use the <ConstString(const char(&other)[N])> constructor
	template<class T>
	const_string(T const * const& other) = delete;

	/* ############### Special member functions ######################################## */
	const_string(const const_string& other) noexcept = default;
	const_string& operator=(const const_string& other) noexcept = default;

	const_string(const_string&& other) noexcept
		: std::string_view(std::exchange(other._as_strview(), std::string_view{}))
		, _data(std::move(other._data))
	{
	}
	const_string& operator=(const_string&& other) noexcept
	{
		this->_as_strview() = std::exchange(other._as_strview(), std::string_view{});
		_data = std::move(other._data);
		return *this;
	}

	/* ################## String functions  ################################# */
	const_string substr(size_t offset = 0, size_t count = npos) const
	{
		const_string retval;
		retval._as_strview() = this->_as_strview().substr(offset, count);
		retval._data = this->_data;
		return retval;
	}

	const_string substr_sentinel(size_t offset, char sentinel) const
	{
		const auto size = this->find(sentinel, offset);
		return substr(offset, size == npos ? this->size() - offset : size - offset);
	}

	bool isZeroTerminated() const
	{
		return this->data()[size()] == '\0';
	}

	const_string unshare() const
	{
		return const_string(static_cast<std::string_view>(*this));
	}

	const_string createZStr() const &
	{
		if (isZeroTerminated()) {
			return *this; //just copy
		} else {
			return unshare();
		}
	}

	const_string createZStr() &&
	{
		if (isZeroTerminated()) {
			return std::move(*this); //already zero terminated - just move
		} else {
			return unshare();
		}
	}

	const char* c_str() const {
		if (!isZeroTerminated()) {
			throw std::runtime_error("Called c_str on ConstString that is not zero terminated -> create zero terminated version wihth createZStr() first");
		}
		return &_as_strview()[0];
	}

	template<class ...ARGS>
	friend const_string concat(const ARGS&...args);

private:
	class atomic_ref_cnt {
		using Cnt_t = std::atomic_int;

	public:
		static constexpr size_t required_space = sizeof(Cnt_t);

		atomic_ref_cnt() = default;
		explicit atomic_ref_cnt(std::unique_ptr<char[]>&& location) noexcept
			: _cnt{ new(location.release()) Cnt_t{ 1 } }
		{
		}
		atomic_ref_cnt(const atomic_ref_cnt& other) noexcept
			: _cnt{ other._cnt }
		{
			_incref();
		}
		atomic_ref_cnt(atomic_ref_cnt&& other) noexcept
			: _cnt{ other._cnt }
		{
			other._cnt = nullptr;
		}
		atomic_ref_cnt& operator=(const atomic_ref_cnt& other) noexcept
		{
			//inc before dec to protect against self assignment
			other._incref();
			_decref();
			_cnt = other._cnt;

			return *this;
		}
		atomic_ref_cnt& operator=(atomic_ref_cnt&& other) noexcept
		{
			_decref();
			_cnt = other._cnt;
			other._cnt = nullptr;
			return *this;
		}
		~atomic_ref_cnt() { _decref(); }

		char* get() noexcept { return reinterpret_cast<char*>(_cnt) + sizeof(Cnt_t); }

		friend void swap(atomic_ref_cnt& l, atomic_ref_cnt& r) noexcept { std::swap(l._cnt, r._cnt); }

	private:
		void _decref() const noexcept
		{
			if (_cnt) {
				if (_cnt->fetch_sub(1) == 1) {
					delete[](reinterpret_cast<char*>(_cnt));
				}
			}
		}
		void _incref() const noexcept
		{
			if (_cnt) {
				_cnt->fetch_add(1, std::memory_order_relaxed);
			}
		}
		Cnt_t* _cnt = nullptr;
	};

	atomic_ref_cnt _data;

	/** private constructor, that takes ownership of a buffer and a size (used in _copyFrom and _concat_impl)
	*
	* Implementation notes:
	* - creating from shared_ptr doesn't bring any advantage, as you can't use std::make_shared for an array
	* - This automatically stores the correct deleter in the shared_ptr (by default shared_ptr<const char> would use delete instead of delete[]
	*/
	const_string(atomic_ref_cnt&& data, size_t size) :
		std::string_view(data.get(), size),
		_data(std::move(data))
	{
	}

	friend void swap(const_string& l, const_string& r)
	{
		using std::swap;
		swap(l._as_strview(), r._as_strview());
		swap(l._data, r._data);
	}

	std::string_view & _as_strview()
	{
		return static_cast<std::string_view&>(*this);
	}

	const std::string_view & _as_strview() const
	{
		return static_cast<const std::string_view&>(*this);
	}

	static inline atomic_ref_cnt _allocate_null_terminated_char_buffer(size_t size)
	{
		atomic_ref_cnt data{ std::make_unique<char[]>(size + 1 + atomic_ref_cnt::required_space) };
		data.get()[size] = '\0'; //zero terminate
		return data;
	}

	void _copyFrom(const std::string_view other)
	{
		if (other.data() == nullptr) {
			this->_as_strview() = std::string_view{};
			return;
		}
		//create buffer and copy data over
		auto data = _allocate_null_terminated_char_buffer(other.size());
		std::copy_n(other.data(), other.size(), data.get());

		//initialize ConstString data fields;
		*this = const_string(std::move(data), other.size());
	}

	//######## impl helper for concat ###############
	static void _addTo(char*& buffer, const std::string_view str)
	{
		std::copy_n(str.data(), str.size(), buffer);
		buffer += str.size();
	}

	template<class ...ARGS>
	inline static size_t _total_size(const ARGS& ...args)
	{
		//c++17: ~ const size_t newSize = 0 + ... + args.size();
		size_t newSize = 0;
		const int ignore1[] = { (newSize += args.size(),0)... };
		(void)ignore1;
		return newSize;
	}

	template<class ...ARGS>
	inline static void _write_to_buffer(char* buffer, const ARGS& ...args)
	{
		const int tignore[] = { (_addTo(buffer,args),0)... };
		(void)tignore;
	}

	template<class ...ARGS>
	inline static const_string _concat_impl(const ARGS& ...args)
	{
		const size_t newSize = _total_size(args ...);
		auto data = _allocate_null_terminated_char_buffer(newSize);
		_write_to_buffer(data.get(), args ...);
		return const_string(std::move(data), newSize);
	}
};


/**
* Function that can concatenate an arbitrary number of objects from which a mart::string_view can be constructed
* returned constStr will always be zero terminated
*/
template<class ...ARGS>
const_string concat(const ARGS& ...args)
{
	return const_string::_concat_impl(std::string_view(args)...);
}

template<class ...ARGS>
std::string concat_cpp_str(const ARGS& ...args)
{
	const size_t newSize = const_string::_total_size(std::string_view(args) ...);

	std::string ret(newSize, ' ');

	const_string::_write_to_buffer(&ret[0], std::string_view(args) ...);

	return ret;
}

inline const const_string& getEmptyConstString()
{
	const static const_string str{};
	return str;
}



#endif