#include <vector>

#include <lambdifier/autodiff.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

#include "catch.hpp"

using namespace lambdifier;
using namespace Catch::literals;

TEST_CASE("compute connections")
{
    // We test the result on a simple polynomial x^2*y + 2
    {
        expression ex = ("x"_var * ("x"_var * "y"_var)) + 2_num;
        auto connections = compute_connections(ex);
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
        auto connections = compute_connections(ex);
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
        auto connections = compute_connections(ex);
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

TEST_CASE("gradient")
{
    // We test that the gradient of x is one
    {
        expression ex = "x"_var;
        auto connections = compute_connections(ex);
        std::unordered_map<std::string, double> point;
        point["x"] = 2.3;
        auto grad = gradient(ex, point, connections);
        REQUIRE(grad["x"] == 1);
    }
    // We test that the gradient of x*y is {x, y}
    {
        expression ex = "x"_var * "y"_var;
        auto connections = compute_connections(ex);
        std::unordered_map<std::string, double> point;
        point["x"] = 2.3;
        point["y"] = 12.43;
        auto grad = gradient(ex, point, connections);
        REQUIRE(grad["x"] == 12.43);
        REQUIRE(grad["y"] == 2.3);
    }
    // We test that the gradient of the mathematical identity sin^2(x) + cos^2(x) = 1 is zero
    {
        expression ex = cos("x"_var) * cos("x"_var) + sin("x"_var) * sin("x"_var);
        auto connections = compute_connections(ex);
        std::unordered_map<std::string, double> point;
        point["x"] = 2.3;
        auto grad = gradient(ex, point, connections);
        REQUIRE(grad["x"] == 0_a);
    }
}