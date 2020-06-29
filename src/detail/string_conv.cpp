#include <cassert>
#include <cstdint>
#include <string>

#include <lambdifier/detail/string_conv.hpp>

namespace lambdifier::detail
{

std::uint32_t uname_to_index(const std::string &s)
{
    assert(s.rfind("u_", 0) == 0);
    return li_from_string<std::uint32_t>(std::string(s.begin() + 2, s.end()));
}

} // namespace lambdifier::detail
