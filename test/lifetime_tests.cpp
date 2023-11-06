//
// Created by Helge Penne on 01/11/2023.
//

#include "owned_ptr.h"

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
    auto t = make_unique<owned_ptr<Target>>(make_owned<Target>());
    ASSERT_FALSE(Target::destroyed);
    t = nullptr;
    ASSERT_TRUE(Target::destroyed);
}

TEST_F(Lifetime, owner_destroyed_before_dep) {
    auto t = make_unique<owned_ptr<Target>>(make_owned<Target>());
    auto dep = t->make_dep();
    ASSERT_FALSE(Target::destroyed);
    t = nullptr;
    ASSERT_TRUE(Target::destroyed);
}

TEST_F(Lifetime, dep_destroyed_before_owner) {
    auto t = make_unique<owned_ptr<Target>>(make_owned<Target>());
    {
        auto dep = t->make_dep();
    }
    ASSERT_FALSE(Target::destroyed);
    t = nullptr;
    ASSERT_TRUE(Target::destroyed);
}
