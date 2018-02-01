#ifndef LOGGER_INL
#define LOGGER_INL


template <class Stream>
void Logger::restart(Stream& stream) noexcept
{
    if constexpr (std::is_same<Stream, std::ifstream>::value)
    {
        stream.clear();
        stream.seekg(0, std::ios::beg);
    }
    else if constexpr (std::is_same<Stream, std::ofstream>::value)
    {
        stream.clear();
        stream.seekp(0, std::ios::beg);
    }
    else
    {
        stream.clear();
    }
}

template <typename T>
void Logger::doWrite(const T& t) noexcept
{
    // It is necessary to flush buffer here.
    outFile << t << ' ' << std::flush;
    _hasNotAnyOutputErrors = outFile.good();
}

template <typename T, typename ...Args>
void Logger::doWrite(const T& t, const Args&... args) noexcept
{
    outFile << t << ' ';
    _hasNotAnyOutputErrors = outFile.good();

    doWrite(args...);
}

template <typename T>
void Logger::doWriteLine(const T& t) noexcept
{
    // std::endl because it is necessary to flush buffer here.
    outFile << t << std::endl;
    _hasNotAnyOutputErrors = outFile.good();
}

template <typename T, typename ...Args>
void Logger::doWriteLine(const T& t, const Args&... args) noexcept
{
    outFile << t << ' ';
    _hasNotAnyOutputErrors = outFile.good();

    doWriteLine(args...);
}

template <typename ...Args>
void Logger::write(const Args&... args) noexcept
{
    std::lock_guard<std::mutex> lockGuard{ _mutex };
    doWrite(args...);
}

template <typename ...Args>
void Logger::writeLine(const Args&... args) noexcept
{
    std::lock_guard<std::mutex> lockGuard{ _mutex };
    doWriteLine(args...);
}

template <typename T>
T Logger::read() noexcept
{  
    std::string readedData;
    if (!inFile.eof())
    {
        std::getline(inFile, readedData);
    }

    bool flag;
    T t = utils::fromString<T>(readedData, flag);

    _hasNotAnyInputErrors = inFile.good() && flag;

    return t;
}

#endif // LOGGER_INL