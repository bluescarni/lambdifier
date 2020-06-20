#include <stdexcept>
#include <string>

#include <lambdifier/detail/check_symbol_name.hpp>

namespace lambdifier::detail
{

void check_symbol_name(const std::string &s)
{
    if (s.find('.') != std::string::npos) {
        throw std::invalid_argument("Invalid symbol name '" + s + "' (the name cannot contain the '.' character)");
    }
}

} // namespace lambdifier::detail
