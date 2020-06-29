#include <vector>

#include <lambdifier/expression.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

#include "catch.hpp"

using namespace lambdifier;
using namespace Catch::literals;

TEST_CASE("equality comparisons")
{
    expression ex1 = "x"_var + 3_num + "y"_var * (cos("x"_var + 3_num)) / pow("x"_var + 3_num, "z"_var + 3_num);
    expression ex2 = "x"_var + 3_num + "y"_var * (cos("x"_var + 3_num)) / pow("x"_var + 3_num, "z"_var + 3_num);
    expression ex3 = "z"_var + 3_num + "y"_var * (cos("x"_var + 3_num)) / pow("x"_var + 3_num, "z"_var + 3_num);
    expression ex4 = "x"_var + 3_num + "y"_var * (cos("x"_var - 3_num)) / pow("x"_var + 3_num, "z"_var + 3_num);
    REQUIRE(ex1 == ex2);
    REQUIRE(ex1 != ex3);
    REQUIRE(ex1 != ex4);
}

TEST_CASE("call operator")
{
    // We test on a number
    {
        expression ex = 2.345_num;
        std::unordered_map<std::string, double> in;
        REQUIRE(ex(in) == 2.345);
    }
    // We test on a variable
    {
        expression ex = "x"_var;
        std::unordered_map<std::string, double> in{{"x", 2.345}};
        REQUIRE(ex(in) == 2.345);
    }
    // We test on a function call
    {
        expression ex = cos("x"_var);
        std::unordered_map<std::string, double> in{{"x", 2.345}};
        REQUIRE(ex(in) == std::cos(2.345));
    }
    // We test on a binary operator
    {
        expression ex = "x"_var / 2.345_num;
        std::unordered_map<std::string, double> in{{"x", 2.345}};
        REQUIRE(ex(in) == 1.);
    }
    // We test on a deeper tree
    {
        expression ex = "x"_var * "y"_var + cos("x"_var * "y"_var);
        std::unordered_map<std::string, double> in{{"x", 2.345}, {"y", -1.}};
        REQUIRE(ex(in) == -2.345 + std::cos(-2.345));
    }
    // We test the corner case of a dictionary not containing the variable.
    {
        expression ex = "x"_var * "y"_var;
        std::unordered_map<std::string, double> in{{"x", 2.345}};
        REQUIRE(ex(in) == 0.);
    }
}

TEST_CASE("compute connections")
{
    // We test the result on a simple polynomial x^2*y + 2
    {
        expression ex = ("x"_var * ("x"_var * "y"_var)) + 2_num;
        auto connections = ex.compute_connections();
        REQUIRE(connections.size() == 7u);
        REQUIRE(connections[0] == std::vector<unsigned>{1, 6});
        REQUIRE(connections[1] == std::vector<unsigned>{2, 3});
        REQUIRE(connections[2] == std::vector<unsigned>{});
        REQUIRE(connections[3] == std::vector<unsigned>{4, 5});
        REQUIRE(connections[4] == std::vector<unsigned>{});
        REQUIRE(connections[5] == std::vector<unsigned>{});
        REQUIRE(connections[6] == std::vector<unsigned>{});
    }
    // We test the result on a known expression with a simple function 2cos(x) + 2yz
    {
        expression ex = cos("x"_var) * 2_num + ("y"_var * "z"_var) * 2_num;
        auto connections = ex.compute_connections();
        REQUIRE(connections.size() == 10u);

        REQUIRE(connections[0] == std::vector<unsigned>{1, 5});
        REQUIRE(connections[1] == std::vector<unsigned>{2, 4});
        REQUIRE(connections[2] == std::vector<unsigned>{3});
        REQUIRE(connections[3] == std::vector<unsigned>{});
        REQUIRE(connections[4] == std::vector<unsigned>{});
        REQUIRE(connections[5] == std::vector<unsigned>{6, 9});
        REQUIRE(connections[6] == std::vector<unsigned>{7, 8});
        REQUIRE(connections[7] == std::vector<unsigned>{});
        REQUIRE(connections[8] == std::vector<unsigned>{});
        REQUIRE(connections[9] == std::vector<unsigned>{});
    }
    // We test the result on a known expression including a multiargument function
    {
        expression ex = pow("x"_var, 2_num) + ("y"_var * "z"_var) * 2_num;
        auto connections = ex.compute_connections();
        REQUIRE(connections.size() == 9u);

        REQUIRE(connections[0] == std::vector<unsigned>{1, 4});
        REQUIRE(connections[1] == std::vector<unsigned>{2, 3});
        REQUIRE(connections[2] == std::vector<unsigned>{});
        REQUIRE(connections[3] == std::vector<unsigned>{});
        REQUIRE(connections[4] == std::vector<unsigned>{5, 8});
        REQUIRE(connections[5] == std::vector<unsigned>{6, 7});
        REQUIRE(connections[6] == std::vector<unsigned>{});
        REQUIRE(connections[7] == std::vector<unsigned>{});
        REQUIRE(connections[8] == std::vector<unsigned>{});
    }
}

TEST_CASE("compute node values")
{
    // We test on a number
    {
        expression ex = 2.345_num;
        std::unordered_map<std::string, double> in;
        auto connections = ex.compute_connections();
        auto node_values = ex.compute_node_values(in, connections);
        REQUIRE(node_values.size() == 1u);
        REQUIRE(node_values[0] == 2.345);
    }
    // We test on a variable
    {
        expression ex = "x"_var;
        std::unordered_map<std::string, double> in{{"x", 2.345}};
        auto connections = ex.compute_connections();
        auto node_values = ex.compute_node_values(in, connections);
        REQUIRE(node_values.size() == 1u);
        REQUIRE(node_values[0] == 2.345);
    }
    // We test on a function call
    {
        expression ex = cos("x"_var);
        std::unordered_map<std::string, double> in{{"x", 2.345}};
        auto connections = ex.compute_connections();
        auto node_values = ex.compute_node_values(in, connections);
        REQUIRE(node_values.size() == 2u);
        REQUIRE(node_values[0] == std::cos(2.345));
        REQUIRE(node_values[1] == 2.345);
    }
    // We test on a binary operator
    {
        expression ex = "x"_var / 2.345_num;
        std::unordered_map<std::string, double> in{{"x", 2.345}};
        auto connections = ex.compute_connections();
        auto node_values = ex.compute_node_values(in, connections);
        REQUIRE(node_values.size() == 3u);
        REQUIRE(node_values[0] == 1);
        REQUIRE(node_values[1] == 2.345);
        // Note here that the tree built (after the simplifictions) is *, "x", 1/2.345
        REQUIRE(node_values[2] == 1. / 2.345);
    }
    // We test on a deeper tree
    {
        expression ex = "x"_var * "y"_var + cos("x"_var * "y"_var);
        std::unordered_map<std::string, double> in{{"x", 2.345}, {"y", -1.}};
        auto connections = ex.compute_connections();
        auto node_values = ex.compute_node_values(in, connections);
        REQUIRE(node_values.size() == 8u);
        REQUIRE(node_values[0] == -2.345 + std::cos(-2.345));
        REQUIRE(node_values[1] == -2.345);
        REQUIRE(node_values[2] == 2.345);
        REQUIRE(node_values[3] == -1.);
        REQUIRE(node_values[4] == std::cos(-2.345));
        REQUIRE(node_values[5] == -2.345);
        REQUIRE(node_values[6] == 2.345);
        REQUIRE(node_values[7] == -1.);
    }
    // We test the corner case of a dictionary not containing the variable.
    {
        expression ex = "x"_var * "y"_var;
        std::unordered_map<std::string, double> in{{"x", 2.345}};
        auto connections = ex.compute_connections();
        auto node_values = ex.compute_node_values(in, connections);
        REQUIRE(node_values.size() == 3u);
        REQUIRE(node_values[0] == 0.);
        REQUIRE(node_values[1] == 2.345);
        REQUIRE(node_values[2] == 0.);
    }
}

TEST_CASE("gradient")
{
    // We test that the gradient of x is one
    {
        expression ex = "x"_var;
        auto connections = ex.compute_connections();
        std::unordered_map<std::string, double> point;
        point["x"] = 2.3;
        auto grad = ex.gradient(point, connections);
        REQUIRE(grad["x"] == 1);
    }
    // We test that the gradient of x*y is {x, y}
    {
        expression ex = "x"_var * "y"_var;
        auto connections = ex.compute_connections();
        std::unordered_map<std::string, double> point;
        point["x"] = 2.3;
        point["y"] = 12.43;
        auto grad = ex.gradient(point, connections);
        REQUIRE(grad["x"] == 12.43);
        REQUIRE(grad["y"] == 2.3);
    }
    // We test that the gradient of the mathematical identity sin^2(x) + cos^2(x) = 1 is zero
    {
        expression ex = cos("x"_var) * cos("x"_var) + sin("x"_var) * sin("x"_var);
        auto connections = ex.compute_connections();
        std::unordered_map<std::string, double> point;
        point["x"] = 2.3;
        auto grad = ex.gradient(point, connections);
        REQUIRE(grad["x"] == 0_a);
    }
}

TEST_CASE("symbolic differentiation")
{
    // We test that the gradient of x is one
    {
        expression ex = "x"_var;
        REQUIRE(ex.diff("x") == 1_num);
    }
}