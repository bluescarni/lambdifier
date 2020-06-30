#ifndef LAMBDIFIER_VARIABLE_HPP
#define LAMBDIFIER_VARIABLE_HPP

#include <cstddef>
#include <string>

#include <llvm/IR/Value.h>

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

class LAMBDIFIER_DLL_PUBLIC variable
{
    std::string name;

public:
    explicit variable(std::string);
    variable(const variable &);
    variable(variable &&) noexcept;
    ~variable();

    // Getter/setter for the name.
    std::string get_name() const;
    void set_name(std::string);

    llvm::Value *codegen(llvm_state &) const;
    std::string to_string() const;
    double evaluate(std::unordered_map<std::string, double> &) const;
    void evaluate(std::unordered_map<std::string, std::vector<double>> &, std::vector<double> &) const;

    void compute_connections(std::vector<std::vector<unsigned>> &, unsigned &) const;
    void compute_node_values(std::unordered_map<std::string, double> &in, std::vector<double> &node_values,
                             const std::vector<std::vector<unsigned>> &node_connections, unsigned &node_counter) const
    {
        node_values[node_counter] = in[get_name()];
        node_counter++;
    }
    void gradient(std::unordered_map<std::string, double> &in, std::unordered_map<std::string, double> &grad,
                  const std::vector<double> &node_values, const std::vector<std::vector<unsigned>> &node_connections,
                  unsigned &node_counter, double acc)
    {
        grad[get_name()] = grad[get_name()] + acc;
        node_counter++;
    }

    expression diff(const std::string &) const;
};

inline namespace literals
{

LAMBDIFIER_DLL_PUBLIC expression operator""_var(const char *, std::size_t);

}

} // namespace lambdifier

#endif
