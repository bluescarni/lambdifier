#ifndef LAMBDIFIER_DETAIL_STRING_CONV_HPP
#define LAMBDIFIER_DETAIL_STRING_CONV_HPP

#include <ios>
#include <locale>
#include <sstream>
#include <string>

namespace lambdifier::detail
{

// Locale-independent to_string()/from_string implementations. See:
// https://stackoverflow.com/questions/1333451/locale-independent-atof
template <typename T>
inline std::string li_to_string(const T &x)
{
    std::ostringstream oss;
    oss.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
    oss.imbue(std::locale("C"));
    oss << x;
    return oss.str();
}

template <typename T>
inline T li_from_string(const std::string &s)
{
    T out(0);
    std::istringstream iss(s);
    iss.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
    iss.imbue(std::locale("C"));
    iss >> out;
    return out;
}

} // namespace lambdifier::detail

#endif
