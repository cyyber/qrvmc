# Changelog

Documentation of all notable changes to the **QRVMC** project.

The format is based on [Keep a Changelog],
and this project adheres to [Semantic Versioning].

## [Unreleased]

### Breaking changes

- The Host interface has two more methods, `get_transient_storage` and
  `set_transient_storage`, for the transient storage of
  [EIP-1153](https://eips.ethereum.org/EIPS/eip-1153), which go-qrl has in
  Zond. They are the last two members of `qrvmc_host_interface`, after
  `access_storage`, and pure virtual methods of `qrvmc::HostInterface`, so
  every Host has to implement them. A key follows the zero-extension
  convention of storage keys and a value is 64 bytes. The Host starts every
  transaction with the transient storage empty and reverts its changes
  together with the rest of the state of a call that did not succeed.
  The ABI version stays 3 until the next release, so the loader does not
  yet tell apart a Host built against older headers, whose interface table
  ends before the new methods a VM may call. Hosts and VMs must be built
  against the same headers.

### Added

- `OP_TLOAD` (`0x5c`) and `OP_TSTORE` (`0x5d`) in `<qrvmc/instructions.h>`
  and in the Zond instruction tables, as in go-qrl's instruction set: each
  costs 100 gas (`WARM_STORAGE_READ_COST`), `TLOAD` replaces the top stack
  item and `TSTORE` takes two.
- `qrvmc::MockedAccount::transient_storage`, which `MockedHost` reads and
  writes in its new Host methods. `MockedHost` does not model transactions:
  clear the map to start another one. The example Host implements the
  methods too.

### Fixed

- `MockedHost::access_account` reports as always warm the addresses of the
  Zond precompiles only, `Q01` to `Q07`, which are the ones go-qrl puts in
  the access list of every transaction. It did so for `Q08` and `Q09` too,
  where go-qrl has no precompile, so the first access to either was 2500 gas
  cheaper in a VM run on `MockedHost` than in go-qrl.

## [3.0.0] — 2026-10-25

### Breaking changes

- The QRVMC ABI version is now 3 (`QRVMC_ABI_VERSION = 3`), following the
  revision renumbering below. The loader rejects a VM built against ABI 2
  headers, and an ABI 2 host rejects a VM built against these, with
  `QRVMC_LOADER_ABI_VERSION_MISMATCH`, instead of loading it and passing
  revision values the VM interprets differently.
- `QRVMC_ZOND` is now `0` (it was `1`), and `QRVMC_MAX_REVISION` and
  `QRVMC_LATEST_STABLE_REVISION` follow it. Under ABI version 2 a host built
  against older headers loaded a newer VM and passed revision `1`, which VMs
  that index per-revision tables read out of range.
- `qrvmc::bytes` and `qrvmc::bytes_view` are now
  `std::basic_string<unsigned char, qrvmc::byte_traits<unsigned char>>` and
  the matching `std::basic_string_view`, defined in `<qrvmc/bytes.hpp>` (as
  in EVMC 11+). libc++ 19 and later deprecate `std::char_traits<uint8_t>`,
  which the previous aliases used. Code that spells
  `std::basic_string<uint8_t>` for QRVMC buffers should use `qrvmc::bytes`.

### Fixed

- Tests and tools configure with CMake 4: Hunter's CLI11 package is built with
  `CMAKE_POLICY_VERSION_MINIMUM=3.5`.

## [2.0.0] — 2026-04-23

### Breaking changes

The QRVMC ABI bumps from 1 to 2 for the 64-byte-address / 512-bit-word
Zond migration. VMs built against `QRVMC_ABI_VERSION = 1` are binary
incompatible with hosts built against this release and vice versa.

- `qrvmc_address.bytes` widened from 20 to **64 bytes** (ML-DSA-87 QRL
  address layout).
- `qrvmc_bytes32.bytes` widened from 32 to **64 bytes** (the 512-bit
  VM word). All APIs taking/returning it — including
  `qrvmc_host_interface.get_storage` / `set_storage` values, log
  topics, `qrvmc_message.create2_salt`, block/transaction context
  fields, `qrvmc_get_block_hash_fn`, code-hash queries — now transmit
  a 64-byte value.
- Types renamed to match the widened sizes: `qrvmc_bytes32` →
  `qrvmc_bytes64` and `qrvmc_uint256be` → `qrvmc_uint512be` in the C
  API; `qrvmc::bytes32` → `qrvmc::bytes64`, `qrvmc::uint256be` →
  `qrvmc::uint512be`, and the `""_bytes32` literal → `""_bytes64` in
  the C++ API.
- Comparison operators on `qrvmc_address` and `qrvmc_bytes64` in the
  C++ header (`qrvmc.hpp`) compare the full widened byte range.

### Notes

- Storage keys stay a 32-byte value semantically (Keccak-256 of a slot
  path); the host-interface signature still accepts `const qrvmc_bytes64*`
  for the key, but callers are expected to zero-extend into the upper
  32 bytes of the wire-level 64-byte struct. Hosts may safely ignore
  the upper 32 bytes when hashing into their own 32-byte key space.
- `QRVMC_ABI_VERSION` check must match exactly between host and VM;
  dynamic-loader callers should reject mismatches on load.

[Unreleased]: https://github.com/theQRL/qrvmc/commits/main
[3.0.0]: https://github.com/theQRL/qrvmc/releases/tag/v3.0.0
[2.0.0]: https://github.com/theQRL/qrvmc/releases/tag/v2.0.0

[Keep a Changelog]: https://keepachangelog.com
[Semantic Versioning]: https://semver.org
