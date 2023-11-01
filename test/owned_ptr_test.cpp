//
// Created by Helge Penne on 01/11/2023.
//

#include "owned_ptr.h"

#include "Bar.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace std;

struct custom_error_handler {
    static void check_condition(bool condition, const char* reason) {
        (void)reason;
        assert(condition);
    }

    static constexpr bool reset_moved_from_dep_ptr{true};
};

TEST(Basics, create)
{
    owned_ptr<string> foo{string{"Foo"}};
    const string* s1 = foo;
    const string* s2 = foo.make_dep();
    ASSERT_EQ(*s1, "Foo");
    ASSERT_EQ(*s2, "Foo");
}

TEST(Basics, custom_error_handler)
{
    owned_ptr<string, custom_error_handler> foo{string{"Foo"}};
    auto dep = dep_ptr<string, custom_error_handler>{foo};
    ASSERT_EQ(*foo, "Foo");
    ASSERT_EQ(*dep, "Foo");
}

TEST(Basics, rule_of_zero) {
    Bar bar;
    auto b = make_owned<int>(1);
}
