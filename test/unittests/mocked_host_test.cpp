// EVMC: Ethereum Client-VM Connector API.
// Copyright 2019 The EVMC Authors.
// Licensed under the Apache License, Version 2.0.

#include <qrvmc/mocked_host.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>

using namespace qrvmc::literals;

static_assert(std::is_copy_constructible_v<qrvmc::MockedHost>);
static_assert(std::is_copy_assignable_v<qrvmc::MockedHost>);
static_assert(std::is_move_constructible_v<qrvmc::MockedHost>);
static_assert(std::is_move_assignable_v<qrvmc::MockedHost>);

TEST(mocked_host, mocked_account)
{
    qrvmc::MockedAccount account;
    EXPECT_EQ(account.nonce, 0);
    --account.nonce;
    account.set_balance(0x0102030405060708);

    EXPECT_EQ(account.balance,
              0x0000000000000000000000000000000000000000000000000102030405060708_bytes64);
    EXPECT_EQ(account.nonce, -1);
}

TEST(mocked_host, recorded_calls_clear_resets_input_copies)
{
    qrvmc::MockedHost host;

    const auto record_call = [&host](const qrvmc::bytes& input) {
        qrvmc_message msg{};
        msg.input_data = input.data();
        msg.input_size = input.size();
        host.call(msg);
    };

    constexpr auto priming_calls = static_cast<size_t>(qrvmc::MockedHost::max_recorded_calls - 1);
    for (auto i = size_t{0}; i < priming_calls; ++i)
    {
        const qrvmc::bytes input{static_cast<uint8_t>(i)};
        record_call(input);
    }

    host.recorded_calls.clear();

    const qrvmc::bytes first_input{0xaa};
    record_call(first_input);
    ASSERT_EQ(host.recorded_calls.size(), size_t{1});
    const auto* first_recorded_input = host.recorded_calls.front().input_data;

    const qrvmc::bytes second_input{0xbb};
    record_call(second_input);
    ASSERT_EQ(host.recorded_calls.size(), size_t{2});

    EXPECT_EQ(host.recorded_calls.front().input_data, first_recorded_input);
    EXPECT_EQ(qrvmc::bytes(host.recorded_calls.front().input_data,
                           host.recorded_calls.front().input_size),
              first_input);
    EXPECT_EQ(qrvmc::bytes(host.recorded_calls.back().input_data,
                           host.recorded_calls.back().input_size),
              second_input);
}

TEST(mocked_host, copy_owns_recorded_call_inputs)
{
    auto orig = std::make_unique<qrvmc::MockedHost>();

    const auto record_call = [](qrvmc::MockedHost& host, const qrvmc::bytes& input) {
        qrvmc_message msg{};
        msg.input_data = input.data();
        msg.input_size = input.size();
        host.call(msg);
    };

    const qrvmc::bytes short_input{1, 2, 3, 4};        // Fits in the string's SSO buffer.
    const qrvmc::bytes long_input(100, uint8_t{0xab});  // Uses heap storage.
    record_call(*orig, short_input);
    record_call(*orig, long_input);
    record_call(*orig, {});

    auto copy = *orig;
    ASSERT_EQ(copy.recorded_calls.size(), size_t{3});

    // The copy must own its input buffers instead of aliasing the source's.
    for (size_t i = 0; i < copy.recorded_calls.size(); ++i)
    {
        const auto& orig_msg = orig->recorded_calls[i];
        const auto& copy_msg = copy.recorded_calls[i];
        ASSERT_EQ(copy_msg.input_size, orig_msg.input_size);
        if (orig_msg.input_size != 0)
        {
            EXPECT_NE(copy_msg.input_data, orig_msg.input_data);
            EXPECT_EQ(qrvmc::bytes(copy_msg.input_data, copy_msg.input_size),
                      qrvmc::bytes(orig_msg.input_data, orig_msg.input_size));
        }
        else
            EXPECT_EQ(copy_msg.input_data, nullptr);
    }

    orig.reset();  // The copy must remain valid after the source is destroyed.
    EXPECT_EQ(qrvmc::bytes(copy.recorded_calls[0].input_data, copy.recorded_calls[0].input_size),
              short_input);
    EXPECT_EQ(qrvmc::bytes(copy.recorded_calls[1].input_data, copy.recorded_calls[1].input_size),
              long_input);

    // Recording more calls on the copy must not invalidate earlier input pointers.
    const auto* stable_input = copy.recorded_calls.front().input_data;
    for (int i = 0; i < 50; ++i)
        record_call(copy, qrvmc::bytes{9, 9, 9});
    EXPECT_EQ(copy.recorded_calls.front().input_data, stable_input);
    EXPECT_EQ(qrvmc::bytes(copy.recorded_calls.front().input_data,
                           copy.recorded_calls.front().input_size),
              short_input);

    // Copy assignment follows the same rules.
    qrvmc::MockedHost assigned;
    assigned = copy;
    ASSERT_EQ(assigned.recorded_calls.size(), copy.recorded_calls.size());
    EXPECT_NE(assigned.recorded_calls.front().input_data, copy.recorded_calls.front().input_data);
    EXPECT_EQ(qrvmc::bytes(assigned.recorded_calls.front().input_data,
                           assigned.recorded_calls.front().input_size),
              short_input);
}

TEST(mocked_host, recorded_call_empty_input_keeps_no_pointer)
{
    qrvmc::MockedHost host;

    const qrvmc::bytes buffer{0xaa};
    qrvmc_message msg{};
    msg.input_data = buffer.data();  // Non-null pointer with zero size is valid per the ABI.
    msg.input_size = 0;
    host.call(msg);

    ASSERT_EQ(host.recorded_calls.size(), size_t{1});
    EXPECT_EQ(host.recorded_calls.front().input_data, nullptr);
    EXPECT_EQ(host.recorded_calls.front().input_size, size_t{0});
}

TEST(mocked_host, storage)
{
    const auto addr1 = qrvmc::address{};
    const auto addr2 = "Q000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000"_address;
    const auto val1 = qrvmc::bytes64{};
    const auto val2 = 0x2000000000000000000000000000000000000000000000000102030405060708_bytes64;
    const auto val3 = 0x1000000000000000000000000000000000000000000000000000000000000000_bytes64;

    qrvmc::MockedHost host;
    const auto& chost = host;

    // Null bytes returned for non-existing accounts.
    EXPECT_EQ(chost.get_storage(addr1, {}), qrvmc::bytes64{});
    EXPECT_EQ(chost.get_storage(addr2, {}), qrvmc::bytes64{});

    // Set storage on non-existing account creates the account.
    EXPECT_EQ(host.set_storage(addr1, val1, val2), QRVMC_STORAGE_ADDED);
    EXPECT_EQ(chost.accounts.count(addr1), 1u);
    EXPECT_EQ(host.accounts[addr1].storage.count(val1), 1u);
    EXPECT_EQ(host.accounts[addr1].storage[val1].current, val2);

    auto& acc2 = host.accounts[addr2];
    EXPECT_EQ(chost.get_storage(addr2, val1), qrvmc::bytes64{});
    EXPECT_EQ(acc2.storage.size(), 0u);
    EXPECT_EQ(host.set_storage(addr2, val1, val2), QRVMC_STORAGE_ADDED);
    EXPECT_EQ(chost.get_storage(addr2, val1), val2);
    EXPECT_EQ(acc2.storage.count(val1), 1u);
    EXPECT_EQ(host.set_storage(addr2, val1, val2), QRVMC_STORAGE_ASSIGNED);
    EXPECT_EQ(chost.get_storage(addr2, val1), val2);
    EXPECT_EQ(acc2.storage.count(val1), 1u);
    EXPECT_EQ(host.set_storage(addr2, val1, val3), QRVMC_STORAGE_ASSIGNED);
    EXPECT_EQ(chost.get_storage(addr2, val1), val3);
    EXPECT_EQ(acc2.storage.count(val1), 1u);
    EXPECT_NE(acc2.storage[val1].current, acc2.storage[val1].original);
    EXPECT_EQ(host.set_storage(addr2, val1, val1), QRVMC_STORAGE_ADDED_DELETED);
    EXPECT_EQ(chost.get_storage(addr2, val1), val1);
    EXPECT_EQ(acc2.storage.count(val1), 1u);
    EXPECT_EQ(acc2.storage.size(), 1u);
    EXPECT_EQ(acc2.storage[val1].current, acc2.storage[val1].original);

    EXPECT_EQ(chost.get_storage(addr2, val3), qrvmc::bytes64{});
    acc2.storage[val3] = val2;
    EXPECT_EQ(chost.get_storage(addr2, val3), val2);
    EXPECT_EQ(acc2.storage.find(val3)->second.current, acc2.storage.find(val3)->second.original);
    EXPECT_EQ(host.set_storage(addr2, val3, val2), QRVMC_STORAGE_ASSIGNED);
    EXPECT_EQ(chost.get_storage(addr2, val3), val2);
    EXPECT_EQ(host.set_storage(addr2, val3, val3), QRVMC_STORAGE_MODIFIED);
    EXPECT_EQ(chost.get_storage(addr2, val3), val3);
    acc2.storage[val3].original = acc2.storage[val3].current;
    EXPECT_EQ(host.set_storage(addr2, val3, val1), QRVMC_STORAGE_DELETED);
    EXPECT_EQ(chost.get_storage(addr2, val3), val1);
}

TEST(mocked_host, storage_update_scenarios)
{
    static constexpr auto addr = "Qff"_address;
    static constexpr auto key = 0xfe_bytes64;

    static constexpr auto execute_scenario = [](const qrvmc::bytes64& original,
                                                const qrvmc::bytes64& current,
                                                const qrvmc::bytes64& value) {
        qrvmc::MockedHost host;
        host.accounts[addr].storage[key] = {current, original};
        return host.set_storage(addr, key, value);
    };

    static constexpr auto O = 0x00_bytes64;
    static constexpr auto X = 0x01_bytes64;
    static constexpr auto Y = 0x02_bytes64;
    static constexpr auto Z = 0x03_bytes64;

    EXPECT_EQ(execute_scenario(O, O, O), QRVMC_STORAGE_ASSIGNED);
    EXPECT_EQ(execute_scenario(X, O, O), QRVMC_STORAGE_ASSIGNED);
    EXPECT_EQ(execute_scenario(O, Y, Y), QRVMC_STORAGE_ASSIGNED);
    EXPECT_EQ(execute_scenario(X, Y, Y), QRVMC_STORAGE_ASSIGNED);
    EXPECT_EQ(execute_scenario(Y, Y, Y), QRVMC_STORAGE_ASSIGNED);
    EXPECT_EQ(execute_scenario(O, Y, Z), QRVMC_STORAGE_ASSIGNED);
    EXPECT_EQ(execute_scenario(X, Y, Z), QRVMC_STORAGE_ASSIGNED);

    EXPECT_EQ(execute_scenario(O, O, Z), QRVMC_STORAGE_ADDED);
    EXPECT_EQ(execute_scenario(X, X, O), QRVMC_STORAGE_DELETED);
    EXPECT_EQ(execute_scenario(X, X, Z), QRVMC_STORAGE_MODIFIED);
    EXPECT_EQ(execute_scenario(X, O, Z), QRVMC_STORAGE_DELETED_ADDED);
    EXPECT_EQ(execute_scenario(X, Y, O), QRVMC_STORAGE_MODIFIED_DELETED);
    EXPECT_EQ(execute_scenario(X, O, X), QRVMC_STORAGE_DELETED_RESTORED);
    EXPECT_EQ(execute_scenario(O, Y, O), QRVMC_STORAGE_ADDED_DELETED);
    EXPECT_EQ(execute_scenario(X, Y, X), QRVMC_STORAGE_MODIFIED_RESTORED);
}
