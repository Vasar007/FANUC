#ifndef UTILITY_INL
#define UTILITY_INL
#pragma once


template <class Container>
void split(const std::string& str, Container& cont, const std::string& delims) noexcept
{
	std::size_t previous	= 0u;
	std::size_t current		= str.find_first_of(delims);

	while (current != std::string::npos)
	{
		cont.push_back(str.substr(previous, current - previous));
		previous	= current + 1u;
		current		= str.find_first_of(delims, previous);
	}

	cont.push_back(str.substr(previous, current - previous));
}

template <typename T>
std::string toString(const T& value) noexcept
{
	std::stringstream stream;

	stream << value;

	return stream.str();
}

template<class T>
std::unique_ptr<T> copyUnique(const std::unique_ptr<T>& source) noexcept
{
	return source ? std::make_unique<T>(*source) : nullptr;
}

template<class T>
void swap(T& first, T& second) noexcept
{
	// Enable ADL (not necessary in our case, but good practice).
	using std::swap;

	if constexpr (isSame<T, Client>() || isSame<T, ServerImitator>())
	{
		swap(first._wsaData,				second._wsaData);
		swap(first._socketSend,				second._socketSend);
		swap(first._socketReceive,			second._socketReceive);
		swap(first._socketSendAddress,		second._socketSendAddress);
		swap(first._socketReceiveAddress,	second._socketReceiveAddress);
		swap(first._addressInfo,			second._addressInfo);
		swap(first._isRunning,				second._isRunning);
		swap(first._isInitialized,			second._isInitialized);
		swap(first._buffer,					second._buffer);
		swap(first._message,				second._message);
	}
	else
	{
		swap(first, second);
	}
}


#endif // UTILITY_INL