#ifndef LAMBDIFIER_DETAIL_STRING_CONV_HPP
#define LAMBDIFIER_DETAIL_STRING_CONV_HPP

#include <cstdint>
#include <ios>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

#include <lambdifier/detail/visibility.hpp>

namespace lambdifier::detail
{

// Locale-independent to_string()/from_string implementations. See:
// https://stackoverflow.com/questions/1333451/locale-independent-atof
template <typename T>
inline std::string li_to_string(const T &x)
{
    std::ostringstream oss;
    oss.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    oss.imbue(std::locale("C"));
    oss << x;
    return oss.str();
}

template <typename T>
inline T li_from_string(const std::string &s)
{
    T out(0);
    std::istringstream iss(s);
    iss.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    iss.imbue(std::locale("C"));
    iss >> out;
    // NOTE: if we did not reach the end of the string,
    // raise an error.
    if (!iss.eof()) {
        throw std::invalid_argument("Error converting the string '" + s + "' to a numerical value");
    }
    return out;
}

// Small helper to compute an index from the name
// of a u variable. E.g., for s = "u_123" this
// will return 123.
LAMBDIFIER_DLL_PUBLIC std::uint32_t uname_to_index(const std::string &);

} // namespace lambdifier::detail

#endif
