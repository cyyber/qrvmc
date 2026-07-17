// EVMC: Ethereum Client-VM Connector API.
// Copyright 2018 The EVMC Authors.
// Licensed under the Apache License, Version 2.0.

#include <qrvmc/helpers.h>

#include <gtest/gtest.h>

// Compile time checks:

static_assert(sizeof(qrvmc_bytes64) == 64, "qrvmc_bytes64 is too big");
static_assert(sizeof(qrvmc_address) == 64, "qrvmc_address is too big");
static_assert(sizeof(qrvmc_vm) <= 64, "qrvmc_vm does not fit cache line");
static_assert(offsetof(qrvmc_message, value) % sizeof(size_t) == 0,
              "qrvmc_message.value not aligned");

// Check enums match int size.
// On GCC/clang the underlying type should be unsigned int, on MSVC int
static_assert(sizeof(qrvmc_call_kind) == sizeof(int),
              "Enum `qrvmc_call_kind` is not the size of int");
static_assert(sizeof(qrvmc_revision) == sizeof(int),
              "Enum `qrvmc_revision` is not the size of int");

static constexpr size_t optionalDataSize =
    sizeof(qrvmc_result) - offsetof(qrvmc_result, create_address);
static_assert(optionalDataSize >= sizeof(qrvmc_result_optional_storage),
              "qrvmc_result's optional data space is too small");

TEST(helpers, release_result)
{
    auto r1 = qrvmc_result{};
    qrvmc_release_result(&r1);

    static qrvmc_result r2;
    static bool e;

    e = false;
    r2 = qrvmc_result{};
    r2.release = [](const qrvmc_result* r) { e = r == &r2; };
    EXPECT_FALSE(e);
    qrvmc_release_result(&r2);
    EXPECT_TRUE(e);
}

TEST(helpers, make_result_rejects_null_output_with_nonzero_size)
{
    const auto result = qrvmc_make_result(QRVMC_SUCCESS, 42, 7, nullptr, 1);

    EXPECT_EQ(result.status_code, QRVMC_FAILURE);
    EXPECT_EQ(result.gas_left, 0);
    EXPECT_EQ(result.gas_refund, 0);
    EXPECT_EQ(result.output_data, nullptr);
    EXPECT_EQ(result.output_size, 0);
    EXPECT_EQ(result.release, nullptr);
}
