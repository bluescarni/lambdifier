#ifndef LAMBDIFIER_MATH_FUNCTIONS_HPP
#define LAMBDIFIER_MATH_FUNCTIONS_HPP

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/expression.hpp>

namespace lambdifier
{

LAMBDIFIER_DLL_PUBLIC expression sin(expression);
LAMBDIFIER_DLL_PUBLIC expression cos(expression);
LAMBDIFIER_DLL_PUBLIC expression tan(expression);

LAMBDIFIER_DLL_PUBLIC expression pow(expression, expression);

LAMBDIFIER_DLL_PUBLIC expression exp(expression);
LAMBDIFIER_DLL_PUBLIC expression exp2(expression);

LAMBDIFIER_DLL_PUBLIC expression log(expression);
LAMBDIFIER_DLL_PUBLIC expression log2(expression);
LAMBDIFIER_DLL_PUBLIC expression log10(expression);

LAMBDIFIER_DLL_PUBLIC expression sqrt(expression);

LAMBDIFIER_DLL_PUBLIC expression abs(expression);

} // namespace lambdifier

#endif
