//
// Created by Helge Penne on 01/11/2023.
//

#include "owned_ptr.h"

#include "Bar.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace std;

TEST(Basics, create)
{
    owned_ptr<string> foo{string{"Foo"}};
    const string* s1 = foo;
    const string* s2 = dep_ptr<string>{foo};
    ASSERT_EQ(*s1, "Foo");
    ASSERT_EQ(*s2, "Foo");
}

TEST(Basics, rule_of_zero) {
    Bar bar;
}
