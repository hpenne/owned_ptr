//
// Created by Helge Penne on 02/11/2023.
//

#include "owned_ptr.h"

#include <exception>
#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace std;

namespace {
    class FailureDetected : public runtime_error {
    public:
        explicit FailureDetected(const string& message) : runtime_error(message) {}
    };

    struct throwing_error_handler {
        static void check_condition(bool condition, const char *reason) {
            if (!condition) {
                throw FailureDetected(reason);
            }
        }

        static constexpr bool reset_when_moved_from{true};
    };

    template<typename T>
    void use(T&& t) {
        (void)t;
    }
}

using ptr = owned_ptr<string, throwing_error_handler>;
using dep = dep_ptr<string, throwing_error_handler>;
using dep_const = dep_ptr_const<string, throwing_error_handler>;

TEST(ErrorHandling, owner_created_and_moved_when_first_is_used_then_error_is_detected) {
    auto first = ptr("foo");
    [[maybe_unused]] auto second{std::move(first)};
    ASSERT_THROW(use(*first), FailureDetected); // NOLINT
    ASSERT_THROW(use(first->length()), FailureDetected);
}

TEST(ErrorHandling, owner_and_dep_created_then_owner_deleted_when_dep_is_referenced_then_error_is_detected) {
    auto foo = make_unique<ptr>("foo");
    auto dep = foo->make_dep();
    const auto dep_const = foo->make_dep();
    foo = nullptr; // NOLINT
    ASSERT_THROW(use(*dep), FailureDetected);
    ASSERT_THROW(use(dep->length()), FailureDetected);
    ASSERT_THROW(use(*dep_const), FailureDetected);
    ASSERT_THROW(use(dep_const->length()), FailureDetected);
}

TEST(ErrorHandling, const_owner_and_dep_created_then_owner_deleted_when_dep_is_referenced_then_error_is_detected) {
    auto dep = [] {
        const auto foo = ptr("foo");
        return foo.make_dep();
    }();
    ASSERT_THROW(use(*dep), FailureDetected);
    ASSERT_THROW(use(dep->length()), FailureDetected);
}

TEST(ErrorHandling, dep_is_moved_from_when_dep_is_dereferenced_then_error_is_detected) {
    auto foo = ptr("foo");
    auto dep = foo.make_dep();
    auto dep2{std::move(dep)}; // NOLINT
    ASSERT_THROW(use(*dep), FailureDetected); // NOLINT
    ASSERT_THROW(use(dep->length()), FailureDetected);
}

TEST(ErrorHandling, const_dep_is_moved_from_when_dep_is_dereferenced_then_error_is_detected) {
    const auto foo = ptr("foo");
    auto dep = foo.make_dep();
    auto dep2{std::move(dep)}; // NOLINT
    dep = std::move(dep);
    ASSERT_EQ(1, foo.num_deps());
    ASSERT_THROW(use(*dep), FailureDetected); // NOLINT
    ASSERT_THROW(use(dep->length()), FailureDetected);
}
