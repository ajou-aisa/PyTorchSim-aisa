#include "npu/NpuConfig.h"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace togsim::npu;

static void check(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

static void expect_invalid(NpuConfig config, const std::string& name,
                           const std::function<void(NpuConfig&)>& mutate) {
  mutate(config);
  try {
    config.validate();
  } catch (const std::invalid_argument&) {
    return;
  }
  throw std::runtime_error("Accepted invalid config: " + name);
}

struct Expected {
  const char* name;
  NpuConfig config;
  uint32_t acc_bank_rows;
  uint64_t spad_row_bytes;
  uint64_t acc_row_bytes;
  uint64_t spad_kib;
  uint64_t acc_kib;
};

static NpuConfig make(IsaRevision revision, uint32_t input_bits, uint32_t array_output_bits,
                      uint32_t dim, uint32_t spad_bank_rows, uint32_t acc_rows,
                      std::optional<PoTALMetadataConfig> potal) {
  NpuConfig c;
  c.isa_revision = revision;
  c.input_bits = input_bits;
  c.array_output_bits = array_output_bits;
  c.accumulator_bits = 32;
  c.dim = dim;
  c.spad_banks = 4;
  c.spad_bank_rows = spad_bank_rows;
  c.acc_banks = 2;
  c.acc_rows = acc_rows;
  c.dma_max_bytes = 64;
  c.dma_bus_bits = 128;
  c.potal = potal;
  return c;
}

int main() {
  // Source-audited geometries (design status document, section 7.2).
  const Expected cases[] = {
      {"gemmini_int8_d16",
       make(IsaRevision::GemminiAisaV1, 8, 20, 16,
            4096, 1024, std::nullopt),
       512, 16, 64, 256, 64},
      {"potal_int8_d16",
       make(IsaRevision::PoTALAisaV1, 8, 20, 16,
            4096, 1024, PoTALMetadataConfig{256, 32, false}),
       512, 16, 64, 256, 64},
      {"potal_int8_d32",
       make(IsaRevision::PoTALAisaV1, 8, 21, 32,
            2048, 512, PoTALMetadataConfig{256, 64, false}),
       256, 32, 128, 256, 64},
      {"potal_int8_d64",
       make(IsaRevision::PoTALAisaV1, 8, 22, 64,
            1024, 256, PoTALMetadataConfig{256, 128, false}),
       128, 64, 256, 256, 64},
      {"potal_int4_d32",
       make(IsaRevision::PoTALAisaV1, 4, 32, 32,
            2048, 512, PoTALMetadataConfig{256, 64, false}),
       256, 16, 128, 128, 64},
  };

  for (const auto& e : cases) {
    e.config.validate();
    check(e.config.acc_bank_rows() == e.acc_bank_rows,
          std::string(e.name) + ": acc bank rows");
    check(e.config.spad_row_bytes() == e.spad_row_bytes,
          std::string(e.name) + ": spad row bytes");
    check(e.config.acc_row_bytes() == e.acc_row_bytes,
          std::string(e.name) + ": acc row bytes");
    check(e.config.spad_capacity_bytes() == e.spad_kib * 1024,
          std::string(e.name) + ": spad capacity");
    check(e.config.acc_capacity_bytes() == e.acc_kib * 1024,
          std::string(e.name) + ": acc capacity");
    std::cout << "CONFIG_PASS " << e.name << "\n";
  }

  const NpuConfig gemmini = cases[0].config;
  const NpuConfig potal = cases[1].config;

  expect_invalid(NpuConfig{}, "all fields missing", [](NpuConfig&) {});
  expect_invalid(gemmini, "missing revision",
                 [](NpuConfig& c) { c.isa_revision = IsaRevision::Unspecified; });
  expect_invalid(gemmini, "input wider than 32 bits",
                 [](NpuConfig& c) { c.input_bits = 33; });
  expect_invalid(gemmini, "SPAD row not whole bytes", [](NpuConfig& c) {
    c.dim = 2;
    c.input_bits = 3;
  });
  expect_invalid(gemmini, "zero dim", [](NpuConfig& c) { c.dim = 0; });
  expect_invalid(gemmini, "zero dma bus",
                 [](NpuConfig& c) { c.dma_bus_bits = 0; });
  expect_invalid(gemmini, "dma max below bus width",
                 [](NpuConfig& c) { c.dma_max_bytes = 8; });
  expect_invalid(gemmini, "array output wider than accumulator",
                 [](NpuConfig& c) { c.array_output_bits = 40; });
  expect_invalid(gemmini, "non-byte accumulator",
                 [](NpuConfig& c) { c.accumulator_bits = 20; });
  expect_invalid(gemmini, "non power-of-two dim",
                 [](NpuConfig& c) { c.dim = 24; });
  expect_invalid(gemmini, "bank smaller than tile",
                 [](NpuConfig& c) { c.spad_bank_rows = 8; });
  expect_invalid(gemmini, "PoTAL metadata on Gemmini",
                 [](NpuConfig& c) { c.potal = PoTALMetadataConfig{256, 32, false}; });
  expect_invalid(potal, "PoTAL without metadata",
                 [](NpuConfig& c) { c.potal.reset(); });
  expect_invalid(potal, "PoTAL zero index entries",
                 [](NpuConfig& c) { c.potal->index_entries_per_buffer = 0; });

  // Option-mode widths without an audited RTL geometry must still be
  // structurally valid; provenance is recorded outside NpuConfig.
  const Expected hypothetical[] = {
      {"gemmini_int4_d32",
       make(IsaRevision::GemminiAisaV1, 4, 16, 32, 2048, 512, std::nullopt),
       256, 16, 128, 128, 64},
      {"potal_int4_d16",
       make(IsaRevision::PoTALAisaV1, 4, 16, 16, 4096, 1024,
            PoTALMetadataConfig{256, 32, false}),
       512, 8, 64, 128, 64},
      {"gemmini_int16_d16",
       make(IsaRevision::GemminiAisaV1, 16, 32, 16, 4096, 1024, std::nullopt),
       512, 32, 64, 512, 64},
  };
  for (const auto& e : hypothetical) {
    e.config.validate();
    check(e.config.spad_row_bytes() == e.spad_row_bytes,
          std::string(e.name) + ": spad row bytes");
    check(e.config.spad_capacity_bytes() == e.spad_kib * 1024,
          std::string(e.name) + ": spad capacity");
    std::cout << "OPTION_PASS " << e.name << "\n";
  }

  std::cout << "NPU_CONFIG_PASS\n";
  return 0;
}
