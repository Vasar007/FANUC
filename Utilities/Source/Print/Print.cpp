#include "Print.h"


namespace utils
{

void stdOutput(const std::string_view str) noexcept
{
    std::cout << str;
}

void fastOutput(const std::string& str) noexcept
{
    const int rc = std::puts(str.c_str());
    if (rc == EOF)
    {
        // POSIX requires that errno is set.
        std::perror("puts()");
    }
}

} // namespace utils