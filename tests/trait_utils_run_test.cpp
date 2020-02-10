/*
Copyright Rene Rivera 2020
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#include <lyra/detail/trait_utils.hpp>
#include "mini_test.hpp"
#include <string>

int main()
{
	using namespace lyra;
	bfg::mini_test::scope test;

	{
		test
			(REQUIRE(
				(!lyra::detail::is_callable<int, int>::value)
			))
			(REQUIRE(
				(!lyra::detail::is_callable<float, int, int>::value)
			))
			(REQUIRE(
				(!lyra::detail::is_callable<std::string, int>::value)
			))
			;
	}
	{
		auto f0 = []() -> bool { return false; };
		auto f1 = [](int x) -> bool { return x > 1; };
		auto f2 = [](int x, float y) -> bool { return x > y; };
		test
			(REQUIRE(
				(lyra::detail::is_callable<decltype(f0)>::value)
			))
			(REQUIRE(
				(lyra::detail::is_callable<decltype(f1), int>::value)
			))
			(REQUIRE(
				(lyra::detail::is_callable<decltype(f2), int, int>::value)
			))
			;
	}

	{
		test
			(REQUIRE(
				(!lyra::detail::is_invocable<int>::value)
			))
			(REQUIRE(
				(!lyra::detail::is_invocable<float>::value)
			))
			(REQUIRE(
				(!lyra::detail::is_invocable<std::string>::value)
			))
			;
	}
	{
		auto f0 = []() -> bool { return false; };
		auto f1 = [](int x) -> bool { return x > 1; };
		auto f2 = [](int x, float y) -> bool { return x > y; };
		test
			(REQUIRE(
				(lyra::detail::is_invocable<decltype(f0)>::value)
			))
			(REQUIRE(
				(lyra::detail::is_invocable<decltype(f1)>::value)
			))
			(REQUIRE(
				(lyra::detail::is_invocable<decltype(f2)>::value)
			))
			;
	}

	return test;
}
