#ifndef PRINT_INL
#define PRINT_INL


template <class OutputStream, typename T>
void print(OutputStream& out, const T& t) noexcept
{
	out << t << ' ';
}

template <class OutputStream, typename T, typename... Args>
void print(OutputStream& out, const T& t, const Args&... args) noexcept
{
	out << t << ' ';
	print(out, args...);
}


template <class OutputStream, typename T>
void println(OutputStream& out, const T& t) noexcept
{
	out << t << '\n';
}

template <class OutputStream, typename T, typename... Args>
void println(OutputStream& out, const T& t, const Args&... args) noexcept
{
	out << t << ' ';
	println(out, args...);
}

template <class T>
constexpr std::string_view typeName() noexcept
{
// Macros used to work on every platform.
#ifdef __clang__
	std::string_view p = __PRETTY_FUNCTION__;
	return std::string_view(p.data() + 33, p.size() - 33 - 1);
#elif defined(__GNUC__)
	std::string_view p = __PRETTY_FUNCTION__;
	#if __cplusplus < 201402
		return std::string_view(p.data() + 36, p.size() - 36 - 1);
	#else
		return std::string_view(p.data() + 48, p.find(';', 49) - 48);
	#endif
#elif defined(_MSC_VER)
	std::string_view p = __FUNCSIG__;
	return std::string_view(p.data() + 83, p.size() - 84 - 6);
#endif
}

#endif // PRINT_INL
