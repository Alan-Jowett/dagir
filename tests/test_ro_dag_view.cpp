/**
 * @file test_ro_dag_view.cpp
 * @brief Unit tests for DagIR ReadOnlyDagView concept and related utilities.
 *
 * @details
 * This test suite validates:
 *  - Compliance of mock types with DagIR concepts (NodeHandle, EdgeRef, ReadOnlyDagView).
 *  - Safe bounds checking for children() and roots() to prevent undefined behavior.
 *  - Utility helpers like BasicEdge and models_read_only_view().
 *
 * @copyright
 * © DagIR Contributors. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#include <catch2/catch_test_macros.hpp>
#include <dagir/ro_dag_view.hpp>
#include <vector>
#include <cstdint>

/**
 * @class MockHandle
 * @brief Minimal handle type for concept testing.
 *
 * @details
 * Implements:
 *  - stable_key() for identity
 *  - debug_address() for diagnostics
 *  - Equality operators
 */
struct MockHandle {
    std::uint64_t id{};
    /// @brief Returns a stable key for memoization.
    constexpr std::uint64_t stable_key() const noexcept { return id; }
    /// @brief Returns a debug address (pointer to self).
    constexpr const void* debug_address() const noexcept { return this; }
    friend constexpr bool operator==(MockHandle a, MockHandle b) noexcept { return a.id == b.id; }
    friend constexpr bool operator!=(MockHandle a, MockHandle b) noexcept { return !(a == b); }
};

/**
 * @class MockEdge
 * @brief Minimal edge wrapper exposing target().
 */
struct MockEdge {
    MockHandle child{};
    /// @brief Returns the child handle.
    constexpr MockHandle target() const noexcept { return child; }
};

/**
 * @class MockDagView
 * @brief Mock adapter modeling ReadOnlyDagView for tests.
 *
 * @details
 * Provides:
 *  - children(handle): range of edges, empty if out-of-bounds.
 *  - roots(): range of handles, empty if roots_ is empty.
 */
class MockDagView {
public:
    using handle = MockHandle;

    /// @brief Constructs a mock DAG view.
    explicit MockDagView(std::vector<handle> roots,
                         std::vector<std::vector<handle>> adjacency)
        : roots_(std::move(roots)), adj_(std::move(adjacency)) {}

    /**
     * @brief Returns range of children for a given handle.
     * @param h Node handle.
     * @return Range of edges or empty range if index is invalid.
     */
    auto children(handle h) const {
        const size_t idx = static_cast<size_t>(h.id);
        std::vector<MockEdge> out;
        if (idx < adj_.size()) {
            out.reserve(adj_[idx].size());
            for (auto const &hh : adj_[idx]) out.push_back(MockEdge{hh});
        }
        return out;
    }

    /**
     * @brief Returns range of roots.
     * @return Range of handles or empty range if roots_ is empty.
     */
    auto roots() const {
        // Return a small vector copy of roots to satisfy range concepts easily.
        std::vector<handle> out = roots_;
        return out;
    }

private:
    std::vector<handle> roots_;
    std::vector<std::vector<handle>> adj_;
};

// -----------------------------
// Unit Tests
// -----------------------------

/**
 * @test Verify NodeHandle concept compliance.
 */
TEST_CASE("MockHandle satisfies NodeHandle concept", "[concepts]") {
    STATIC_REQUIRE(dagir::NodeHandle<MockHandle>);
    MockHandle h{42};
    REQUIRE(h.stable_key() == 42);
    REQUIRE(h.debug_address() == &h);
}

/**
 * @test Verify EdgeRef concept compliance.
 */
TEST_CASE("MockEdge satisfies EdgeRef concept", "[concepts]") {
    STATIC_REQUIRE(dagir::EdgeRef<MockEdge, MockHandle>);
    MockHandle h{7};
    MockEdge e{h};
    REQUIRE(e.target().stable_key() == 7);
}

/**
 * @test Verify ReadOnlyDagView concept compliance and helper.
 */
TEST_CASE("MockDagView satisfies ReadOnlyDagView concept", "[concepts]") {
    STATIC_REQUIRE(dagir::ReadOnlyDagView<MockDagView>);
    STATIC_REQUIRE(dagir::models_read_only_view<MockDagView>());

    MockHandle root{0}, child{1};
    MockDagView view({root}, {{child}, {}});

    auto roots = view.roots();
    REQUIRE((*roots.begin()).stable_key() == 0);

    auto children = view.children(root);
    REQUIRE((*children.begin()).target().stable_key() == 1);

    // Bounds check: invalid handle should yield empty range
    MockHandle invalid{99};
    auto emptyChildren = view.children(invalid);
    REQUIRE(emptyChildren.begin() == emptyChildren.end());
}

/**
 * @test Verify empty roots returns empty range.
 */
TEST_CASE("Empty roots returns empty range", "[bounds]") {
    MockDagView emptyView({}, {});
    auto emptyRoots = emptyView.roots();
    REQUIRE(emptyRoots.begin() == emptyRoots.end());
}

/**
 * @test Verify BasicEdge utility works with NodeHandle.
 */
TEST_CASE("BasicEdge wrapper returns correct target", "[utility]") {
    dagir::BasicEdge<MockHandle> edge{ MockHandle{99} };
    REQUIRE(edge.target().stable_key() == 99);
}
