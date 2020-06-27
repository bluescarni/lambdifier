#ifndef LAMBDIFIER_AUTODIFF_HPP
#define LAMBDIFIER_AUTODIFF_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/expression.hpp>

namespace lambdifier
{
// Computes the node connections for the expression tree
LAMBDIFIER_DLL_PUBLIC std::vector<std::vector<unsigned>> compute_connections(const expression &);
// Computes the gradient of the expression using reverse mode automatic differentiation
LAMBDIFIER_DLL_PUBLIC std::unordered_map<std::string, double>
gradient(const expression &, std::unordered_map<std::string, double> &, const std::vector<std::vector<unsigned>> &);

} // namespace lambdifier

#endif
