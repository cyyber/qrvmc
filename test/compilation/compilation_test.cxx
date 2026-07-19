// EVMC: Ethereum Client-VM Connector API.
// Copyright 2018 The EVMC Authors.
// Licensed under the Apache License, Version 2.0.

// Test compilation of C and C++ public headers.

#include <qrvmc/qrvmc.h>
#include <qrvmc/qrvmc.hpp>
#include <qrvmc/filter_iterator.hpp>
#include <qrvmc/helpers.h>
#include <qrvmc/hex.hpp>
#include <qrvmc/instructions.h>
#include <qrvmc/loader.h>
#include <qrvmc/mocked_host.hpp>
#include <qrvmc/utils.h>

#include <iterator>
#include <utility>
#include <vector>

// Include again to check if headers have proper include guards.
#include <qrvmc/qrvmc.h>               //NOLINT(readability-duplicate-include)
#include <qrvmc/qrvmc.hpp>             //NOLINT(readability-duplicate-include)
#include <qrvmc/filter_iterator.hpp>  //NOLINT(readability-duplicate-include)
#include <qrvmc/helpers.h>            //NOLINT(readability-duplicate-include)
#include <qrvmc/hex.hpp>              //NOLINT(readability-duplicate-include)
#include <qrvmc/instructions.h>       //NOLINT(readability-duplicate-include)
#include <qrvmc/loader.h>             //NOLINT(readability-duplicate-include)
#include <qrvmc/mocked_host.hpp>      //NOLINT(readability-duplicate-include)
#include <qrvmc/utils.h>              //NOLINT(readability-duplicate-include)

static_assert(!noexcept(qrvmc::hex(uint8_t{})));
static_assert(!noexcept(qrvmc::from_hex("00", "00" + 2, static_cast<uint8_t*>(nullptr))));
static_assert(!noexcept(qrvmc::from_spaced_hex("00")));

#if __cplusplus >= 202002L
namespace
{
bool is_positive(int x) noexcept
{
    return x > 0;
}
}  // namespace

using filter_iter = qrvmc::filter_iterator<std::vector<int>::const_iterator, is_positive>;

static_assert(std::input_iterator<filter_iter>);
static_assert(!noexcept(filter_iter{std::declval<std::vector<int>::const_iterator>(),
                                    std::declval<std::vector<int>::const_iterator>()}));
static_assert(!noexcept(*std::declval<const filter_iter&>()));
static_assert(!noexcept(++std::declval<filter_iter&>()));
static_assert(!noexcept(std::declval<filter_iter&>()++));
static_assert(!noexcept(std::declval<const filter_iter&>() == std::declval<const filter_iter&>()));
#endif
