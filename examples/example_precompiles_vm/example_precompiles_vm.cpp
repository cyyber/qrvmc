// EVMC: Ethereum Client-VM Connector API.
// Copyright 2019 The EVMC Authors.
// Licensed under the Apache License, Version 2.0.

#include "example_precompiles_vm.h"
#include <qrvmc/helpers.h>
#include <algorithm>
#include <cstdint>
#include <limits>

namespace
{
qrvmc_result execute_identity(const qrvmc_message* msg)
{
    auto result = qrvmc_result{};

    // Check the gas cost.
    constexpr auto identity_base_gas = int64_t{15};
    constexpr auto identity_word_gas = int64_t{3};
    constexpr auto word_size = size_t{32};
    constexpr auto max_input_words =
        static_cast<std::uint64_t>((std::numeric_limits<int64_t>::max() - identity_base_gas) /
                                   identity_word_gas);

    const auto input_words =
        static_cast<std::uint64_t>(msg->input_size / word_size) + (msg->input_size % word_size != 0);
    if (input_words > max_input_words)
    {
        result.status_code = QRVMC_OUT_OF_GAS;
        return result;
    }

    const auto gas_cost = identity_base_gas + identity_word_gas * static_cast<int64_t>(input_words);
    if (msg->gas < gas_cost)
    {
        result.status_code = QRVMC_OUT_OF_GAS;
        return result;
    }

    return qrvmc_make_result(
        QRVMC_SUCCESS, msg->gas - gas_cost, 0, msg->input_data, msg->input_size);
}

qrvmc_result execute_empty(const qrvmc_message* msg)
{
    auto result = qrvmc_result{};
    result.status_code = QRVMC_SUCCESS;
    result.gas_left = msg->gas;
    return result;
}

qrvmc_result not_implemented()
{
    auto result = qrvmc_result{};
    result.status_code = QRVMC_REJECTED;
    return result;
}

qrvmc_result execute(qrvmc_vm* /*vm*/,
                     const qrvmc_host_interface* /*host*/,
                     qrvmc_host_context* /*context*/,
                     enum qrvmc_revision /*rev*/,
                     const qrvmc_message* msg,
                     const uint8_t* /*code*/,
                     size_t /*code_size*/)
{
    // The EIP-1352 (https://eips.ethereum.org/EIPS/eip-1352) defines
    // the range 0 - Qffff (2 bytes) of addresses reserved for precompiled contracts.
    // Check if the code address is within the reserved range.

    constexpr auto prefix_size = sizeof(qrvmc_address) - 2;
    const auto& addr = msg->code_address;
    // Check if the address prefix is all zeros.
    if (std::any_of(&addr.bytes[0], &addr.bytes[prefix_size], [](uint8_t x) { return x != 0; }))
    {
        // If not, reject the execution request.
        auto result = qrvmc_result{};
        result.status_code = QRVMC_REJECTED;
        return result;
    }

    // Extract the precompiled contract id from last 2 bytes of the code address.
    const auto id = (addr.bytes[prefix_size] << 8) | addr.bytes[prefix_size + 1];
    switch (id)
    {
    case 0x0001:  // DEPOSITROOT
    case 0x0002:  // SHA256
    case 0x0004:  // Identity
        return execute_identity(msg);

    case 0x0005:  // EXPMOD
        return not_implemented();

    default:  // As if empty code was executed.
        return execute_empty(msg);
    }
}
}  // namespace

qrvmc_vm* qrvmc_create_example_precompiles_vm()
{
    static struct qrvmc_vm vm = {
        QRVMC_ABI_VERSION,
        "example_precompiles_vm",
        PROJECT_VERSION,
        [](qrvmc_vm*) {},
        execute,
        [](qrvmc_vm*) { return qrvmc_capabilities_flagset{QRVMC_CAPABILITY_PRECOMPILES}; },
        nullptr,
    };
    return &vm;
}
