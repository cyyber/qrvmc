# Changelog

Documentation of all notable changes to the **QRVMC** project.

The format is based on [Keep a Changelog],
and this project adheres to [Semantic Versioning].

## [2.0.0] — 2026-04-23

### Breaking changes

The QRVMC ABI bumps from 1 to 2 for the 48-byte-address / 512-bit-word
Zond migration. VMs built against `QRVMC_ABI_VERSION = 1` are binary
incompatible with hosts built against this release and vice versa.

- `qrvmc_address.bytes` widened from 20 to **48 bytes** (ML-DSA-87 QRL
  address layout).
- `qrvmc_bytes32.bytes` widened from 32 to **64 bytes** (the 512-bit
  VM word). All APIs taking/returning `qrvmc_bytes32` — including
  `qrvmc_host_interface.get_storage` / `set_storage` values, log
  topics, `qrvmc_message.create2_salt`, block/transaction context
  fields, `qrvmc_get_block_hash_fn`, code-hash queries — now transmit
  a 64-byte value.
- Comparison operators on `qrvmc_address` and `qrvmc_bytes32` in the
  C++ header (`qrvmc.hpp`) compare the full widened byte range.

### Notes

- Storage keys stay a 32-byte value semantically (Keccak-256 of a slot
  path); the host-interface signature still accepts `const qrvmc_bytes32*`
  for the key, but callers are expected to zero-extend into the upper
  32 bytes of the wire-level 64-byte struct. Hosts may safely ignore
  the upper 32 bytes when hashing into their own 32-byte key space.
- `QRVMC_ABI_VERSION` check must match exactly between host and VM;
  dynamic-loader callers should reject mismatches on load.

[2.0.0]: https://github.com/adamtka42/qrvmc/compare/64-byte-word

[Keep a Changelog]: https://keepachangelog.com
[Semantic Versioning]: https://semver.org
