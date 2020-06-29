#ifndef LAMBDIFIER_NUMBER_HPP
#define LAMBDIFIER_NUMBER_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include <llvm/IR/Value.h>

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

class LAMBDIFIER_DLL_PUBLIC number
{
    double value;

public:
    explicit number(double);
    number(const number &);
    number(number &&) noexcept;
    ~number();

    double get_value() const;
    void set_value(double);

    llvm::Value *codegen(llvm_state &) const;
    std::string to_string() const;
    double evaluate(std::unordered_map<std::string, double> &) const;
    void evaluate(std::unordered_map<std::string, std::vector<double>> &, std::vector<double> &) const;
    void compute_connections(std::vector<std::vector<unsigned>> &node_connections, unsigned &node_counter) const;
    void compute_node_values(std::unordered_map<std::string, double> &in, std::vector<double> &node_values,
                             const std::vector<std::vector<unsigned>> &node_connections, unsigned &node_counter) const
    {
        node_values[node_counter] = get_value();
        node_counter++;
    }
    void gradient(std::unordered_map<std::string, double> &in, std::unordered_map<std::string, double> &grad,
                  const std::vector<double> &node_values, const std::vector<std::vector<unsigned>> &node_connections,
                  unsigned &node_counter, double acc)
    {
        node_counter++;
    }
    expression diff(const std::string &) const;
};

inline namespace literals
{

LAMBDIFIER_DLL_PUBLIC expression operator""_num(long double);
LAMBDIFIER_DLL_PUBLIC expression operator""_num(unsigned long long);

} // namespace literals

} // namespace lambdifier

#endif
