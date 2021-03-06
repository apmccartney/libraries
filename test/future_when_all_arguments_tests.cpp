/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/**************************************************************************************************/

#include <boost/test/unit_test.hpp>

#include <stlab/concurrency/default_executor.hpp>
#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/utility.hpp>

#include <string>

#include "future_test_helper.hpp"

using namespace stlab;
using namespace future_test_helper;

BOOST_FIXTURE_TEST_SUITE(future_when_all_args_int, test_fixture<int>)
BOOST_AUTO_TEST_CASE(future_when_all_args_int_with_one_element) {
    BOOST_TEST_MESSAGE("running future when_all int with one element");

    auto f1 = async(custom_scheduler<0>(), [] { return 42; });
    sut = when_all(custom_scheduler<1>(), [](auto x) { return x + x; }, f1);

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, *sut.get_try());
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_when_all_args_int_with_many_elements) {
    BOOST_TEST_MESSAGE("running future when_all args int with many elements");

    auto f1 = async(custom_scheduler<0>(), [] { return 1; });
    auto f2 = async(custom_scheduler<0>(), [] { return 2; });
    auto f3 = async(custom_scheduler<0>(), [] { return 3; });
    auto f4 = async(custom_scheduler<0>(), [] { return 5; });

    sut = when_all(
        custom_scheduler<1>(),
        [](int x1, int x2, int x3, int x4) { return 7 * x1 + 11 * x2 + 13 * x3 + 17 * x4; }, f1, f2,
        f3, f4);

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(1 * 7 + 2 * 11 + 3 * 13 + 5 * 17, *sut.get_try());
    BOOST_REQUIRE_LE(4, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_when_all_args_int_with_ready_element) {
    BOOST_TEST_MESSAGE("running future when_all int with ready element");

    sut = when_all(custom_scheduler<1>(), [](auto x) { return x + x; }, make_ready_future<int>(42, immediate_executor));

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, *sut.get_try());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_when_all_args_int_with_two_ready_element) {
    BOOST_TEST_MESSAGE("running future when_all int with two ready element");

    sut = when_all(custom_scheduler<1>(), [](auto x, auto y) { return x + y; },
                   make_ready_future<int>(42, immediate_executor),
                   make_ready_future<int>(42, immediate_executor));

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(42 + 42, *sut.get_try());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_when_all_args) {

    auto main_thread_id = std::this_thread::get_id();
    auto sut = when_all(custom_scheduler<1>(), [] { 
        return std::this_thread::get_id(); 
    }, make_ready_future(stlab::immediate_executor));

    wait_until_future_completed(sut);

    BOOST_REQUIRE(main_thread_id != *sut.get_try());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(future_when_all_args_string, test_fixture<std::string>)
BOOST_AUTO_TEST_CASE(future_when_all_args_with_different_types) {
    BOOST_TEST_MESSAGE("running future when_all args with different types");

    auto f1 = async(custom_scheduler<0>(), [] { return 1; });
    auto f2 = async(custom_scheduler<0>(), [] { return 3.1415; });
    auto f3 = async(custom_scheduler<0>(), [] { return std::string("Don't panic!"); });
    auto f4 = async(custom_scheduler<0>(), [] { return std::vector<size_t>(2, 3); });

    sut = when_all(custom_scheduler<1>(),
                   [](int x1, double x2, const std::string& x3, const std::vector<size_t>& x4) {
                       std::stringstream st;
                       st << x1 << " " << x2 << " " << x3 << " " << x4[0] << " " << x4[1];
                       return st.str();
                   },
                   f1, f2, f3, f4);

    check_valid_future(sut);
    wait_until_future_completed(sut);

    BOOST_REQUIRE_EQUAL(std::string("1 3.1415 Don't panic! 3 3"), *sut.get_try());
    BOOST_REQUIRE_LE(4, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}
BOOST_AUTO_TEST_SUITE_END()

// ----------------------------------------------------------------------------
//                             Error cases
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(future_when_all_args_int_failure, test_fixture<int>)
BOOST_AUTO_TEST_CASE(future_when_all_args_int_failure_with_one_element) {
    BOOST_TEST_MESSAGE("running future when_all int with range of one element");

    auto f1 = async(custom_scheduler<0>(), []() -> int { throw test_exception("failure"); });
    sut = when_all(custom_scheduler<1>(), [](auto x) { return x + x; }, f1);

    wait_until_future_fails<test_exception>(sut);

    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_LE(1, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_when_all_args_int_with_many_elements_one_failing) {
    BOOST_TEST_MESSAGE("running future when_all args int with many elements one failing");

    auto f1 = async(custom_scheduler<0>(), [] { return 1; });
    auto f2 = async(custom_scheduler<0>(), []() -> int { throw test_exception("failure"); });
    auto f3 = async(custom_scheduler<0>(), [] { return 3; });
    auto f4 = async(custom_scheduler<0>(), [] { return 5; });

    sut = when_all(
        custom_scheduler<1>(),
        [](int x1, int x2, int x3, int x4) { return 7 * x1 + 11 * x2 + 13 * x3 + 17 * x4; }, f1, f2,
        f3, f4);

    wait_until_future_fails<test_exception>(sut);

    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_LE(4, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_when_all_args_int_with_many_elements_all_failing) {
    BOOST_TEST_MESSAGE("running future when_all args int with many elements all failing");

    auto f1 = async(custom_scheduler<0>(), []() -> int { throw test_exception("failure"); });
    auto f2 = async(custom_scheduler<0>(), []() -> int { throw test_exception("failure"); });
    auto f3 = async(custom_scheduler<0>(), []() -> int { throw test_exception("failure"); });
    auto f4 = async(custom_scheduler<0>(), []() -> int { throw test_exception("failure"); });

    sut = when_all(
        custom_scheduler<1>(),
        [](int x1, int x2, int x3, int x4) { return 7 * x1 + 11 * x2 + 13 * x3 + 17 * x4; }, f1, f2,
        f3, f4);

    wait_until_future_fails<test_exception>(sut);

    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_LE(4, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(future_when_all_args_string_failure, test_fixture<std::string>)
BOOST_AUTO_TEST_CASE(future_when_all_args_with_different_types_one_failing) {
    BOOST_TEST_MESSAGE("running future when_all args with different types one failing");

    auto f1 = async(custom_scheduler<0>(), [] { return 1; });
    auto f2 = async(custom_scheduler<0>(), [] { return 3.1415; });
    auto f3 =
        async(custom_scheduler<0>(), []() -> std::string { throw test_exception("failure"); });
    auto f4 = async(custom_scheduler<0>(), [] { return std::vector<size_t>(2, 3); });

    sut = when_all(custom_scheduler<1>(),
                   [](int x1, double x2, const std::string& x3, const std::vector<size_t>& x4) {
                       std::stringstream st;
                       st << x1 << " " << x2 << " " << x3 << " " << x4[0] << " " << x4[1];
                       return st.str();
                   },
                   f1, f2, f3, f4);

    wait_until_future_fails<test_exception>(sut);

    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_LE(4, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_CASE(future_when_all_args_with_different_types_all_failing) {
    BOOST_TEST_MESSAGE("running future when_all args with different types all failing");

    auto f1 = async(custom_scheduler<0>(), []() -> int { throw test_exception("failure"); });
    auto f2 = async(custom_scheduler<0>(), []() -> double { throw test_exception("failure"); });
    auto f3 =
        async(custom_scheduler<0>(), []() -> std::string { throw test_exception("failure"); });
    auto f4 = async(custom_scheduler<0>(),
                    []() -> std::vector<size_t> { throw test_exception("failure"); });

    sut = when_all(custom_scheduler<1>(),
                   [](int x1, double x2, const std::string& x3, const std::vector<size_t>& x4) {
                       std::stringstream st;
                       st << x1 << " " << x2 << " " << x3 << " " << x4[0] << " " << x4[1];
                       return st.str();
                   },
                   f1, f2, f3, f4);

    wait_until_future_fails<test_exception>(sut);

    check_failure<test_exception>(sut, "failure");
    BOOST_REQUIRE_LE(4, custom_scheduler<0>::usage_counter());
    BOOST_REQUIRE_LE(1, custom_scheduler<1>::usage_counter());
}

BOOST_AUTO_TEST_SUITE_END()
