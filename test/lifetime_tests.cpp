//
// Created by Helge Penne on 01/11/2023.
//

#include "owned_ptr.h"

#include <optional>

#include <gtest/gtest.h>

using namespace std;

struct Target {
    static bool destroyed;

    ~Target() { destroyed = true; }
};

bool Target::destroyed{false};

struct Lifetime : public testing::Test {
    Lifetime() {
        Target::destroyed = false;
    }
};

TEST_F(Lifetime, create_and_destroy) {
    optional<owned_ptr<Target>> t = make_owned<Target>();
    ASSERT_FALSE(Target::destroyed);
    t = nullopt;
    ASSERT_TRUE(Target::destroyed);
}

TEST_F(Lifetime, owner_destroyed_before_dep) {
    optional<owned_ptr<Target>> t = make_owned<Target>();
    auto dep = t->make_dep();
    ASSERT_FALSE(Target::destroyed);
    t = nullopt;
    ASSERT_TRUE(Target::destroyed);
}

TEST_F(Lifetime, dep_destroyed_before_owner) {
    optional<owned_ptr<Target>> t = make_owned<Target>();
    {
        auto dep = t->make_dep();
    }
    ASSERT_FALSE(Target::destroyed);
    t = nullopt;
    ASSERT_TRUE(Target::destroyed);
}
