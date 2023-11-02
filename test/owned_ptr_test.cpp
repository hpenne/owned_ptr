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
    static void check_condition(bool condition, const char *reason) {
        (void) reason;
        assert(condition);
    }

    static constexpr bool reset_moved_from_dep_ptr{false};
};

TEST(Basics, create_and_deref) {
    auto foo = make_owned<string>("Foo");
    auto dep1 = foo.make_dep();
    ASSERT_EQ(*dep1, "Foo");
    const auto dep2 = foo.make_dep();
    ASSERT_EQ(*dep2, "Foo");
}

TEST(Basics, create_and_deref_const) {
    const owned_ptr<string> foo{string{"Foo"}};
    auto dep1 = foo.make_dep();
    ASSERT_EQ(*dep1, "Foo");
    const auto dep2 = foo.make_dep();
    ASSERT_EQ(*dep2, "Foo");
}

TEST(Basics, custom_error_handler) {
    owned_ptr<string, custom_error_handler> foo{string{"Foo"}};
    {
        auto dep1 = dep_ptr<string, custom_error_handler>{foo};
        auto dep2 = foo.make_dep();
        ASSERT_EQ(2, foo.num_deps());
        ASSERT_EQ(*foo, "Foo");
        ASSERT_EQ(*dep1, "Foo");
        ASSERT_EQ(*dep2, "Foo");
    }
    ASSERT_EQ(0, foo.num_deps());
}

TEST(Basics, special_member_functions) {
    owned_ptr<string> foo{string{"Foo"}};
    owned_ptr<string> foo_b{string{"FooB"}};
    auto dep = foo.make_dep();
    auto dep2 = std::move(dep);
    ASSERT_EQ(1, foo.num_deps());
    auto dep3{dep2};
    ASSERT_EQ(2, foo.num_deps());
    [[maybe_unused]] auto dep4 = dep3;
    auto dep_b = foo_b.make_dep();
    dep4 = dep_b;
    ASSERT_EQ(2, foo.num_deps());
    ASSERT_EQ(2, foo_b.num_deps());
    [[maybe_unused]] auto dep5{std::move(dep2)};
    ASSERT_EQ(2, foo.num_deps());
    [[maybe_unused]] auto dep6{dep5};
    dep6 = foo_b.make_dep();
    ASSERT_EQ(2, foo.num_deps());
    ASSERT_EQ(3, foo_b.num_deps());
}

TEST(Basics, special_member_functions_2) {
    owned_ptr<string, custom_error_handler> foo{string{"Foo"}};
    owned_ptr<string, custom_error_handler> foo_b{string{"FooB"}};
    auto dep = foo.make_dep();
    auto dep2 = std::move(dep);
    ASSERT_EQ(2, foo.num_deps());
    auto dep3{dep2};
    ASSERT_EQ(3, foo.num_deps());
    [[maybe_unused]] auto dep4 = dep3;
    auto dep_b = foo_b.make_dep();
    dep4 = dep_b;
    ASSERT_EQ(3, foo.num_deps());
    ASSERT_EQ(2, foo_b.num_deps());
    [[maybe_unused]] auto dep5{std::move(dep2)};
    ASSERT_EQ(4, foo.num_deps());
    [[maybe_unused]] auto dep6{dep5};
    dep6 = foo_b.make_dep();
    ASSERT_EQ(4, foo.num_deps());
    ASSERT_EQ(3, foo_b.num_deps());
}

TEST(Basics, special_member_functions_const) {
    const owned_ptr<string> foo{string{"Foo"}};
    const owned_ptr<string> foo_b{string{"FooB"}};
    auto dep = foo.make_dep();
    auto dep2 = std::move(dep);
    ASSERT_EQ(1, foo.num_deps());
    auto dep3{dep2};
    ASSERT_EQ(2, foo.num_deps());
    [[maybe_unused]] auto dep4 = dep3;
    auto dep_b = foo_b.make_dep();
    dep4 = dep_b;
    ASSERT_EQ(2, foo.num_deps());
    ASSERT_EQ(2, foo_b.num_deps());
    [[maybe_unused]] auto dep5{std::move(dep2)};
    ASSERT_EQ(2, foo.num_deps());
    [[maybe_unused]] auto dep6{dep5};
    dep6 = foo_b.make_dep();
    ASSERT_EQ(2, foo.num_deps());
    ASSERT_EQ(3, foo_b.num_deps());
}

TEST(Basics, special_member_functions_2_const) {
    const owned_ptr<string, custom_error_handler> foo{string{"Foo"}};
    const owned_ptr<string, custom_error_handler> foo_b{string{"FooB"}};
    auto dep = foo.make_dep();
    auto dep2 = std::move(dep);
    ASSERT_EQ(2, foo.num_deps());
    auto dep3{dep2};
    ASSERT_EQ(3, foo.num_deps());
    [[maybe_unused]] auto dep4 = dep3;
    auto dep_b = foo_b.make_dep();
    dep4 = dep_b;
    ASSERT_EQ(3, foo.num_deps());
    ASSERT_EQ(2, foo_b.num_deps());
    [[maybe_unused]] auto dep5{std::move(dep2)};
    ASSERT_EQ(4, foo.num_deps());
    [[maybe_unused]] auto dep6{dep5};
    dep6 = foo_b.make_dep();
    ASSERT_EQ(4, foo.num_deps());
    ASSERT_EQ(3, foo_b.num_deps());
}

TEST(Basics, rule_of_zero) {
    Bar bar{42};
    ASSERT_EQ(42, bar.get_value());
}

TEST(Basics, arrow) {
    auto bar = owned_ptr<Bar>::make(42);
    ASSERT_EQ(42, bar->get_value());
    auto dep1 = bar.make_dep();
    ASSERT_EQ(42, dep1->get_value());
    const auto dep2 = bar.make_dep();
    ASSERT_EQ(42, dep2->get_value());
}

TEST(Basics, arrow_const) {
    const auto bar = owned_ptr<Bar>::make(42);
    ASSERT_EQ(42, bar->get_value());
    auto dep1 = bar.make_dep();
    ASSERT_EQ(42, dep1->get_value());
    const auto dep2 = bar.make_dep();
    ASSERT_EQ(42, dep2->get_value());
}
