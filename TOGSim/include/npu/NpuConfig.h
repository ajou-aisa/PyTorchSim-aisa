#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>

#include "npu/isa/Isa.h"

namespace togsim::npu
{
    struct PoTALMetadataConfig
    {
        uint32_t scale_entries_per_column = 0;
        uint32_t index_entries_per_buffer = 0;
        bool index_queue_enabled = false;
    };

    // All hardware values are supplied by the caller (YAML or gem5 adapter).
    // Zero/Unspecified means missing, not a hardware default. Bit widths are
    // free options; SPAD rows are bit-packed (element i at bit i*input_bits),
    // which equals byte arrays for 8-bit and PoTAL W4 packing ABI 1 for 4-bit.
    // validate() checks structure only; RTL provenance and arithmetic/timing
    // support belong to the Device and experiment records.
    struct NpuConfig
    {
        IsaRevision isa_revision = IsaRevision::Unspecified;
        uint32_t input_bits = 0, array_output_bits = 0, accumulator_bits = 0;
        uint32_t dim = 0;
        uint32_t spad_banks = 0, spad_bank_rows = 0;
        uint32_t acc_banks = 0, acc_rows = 0; // acc_rows = TOTAL
        uint32_t dma_max_bytes = 0, dma_bus_bits = 0;
        std::optional<PoTALMetadataConfig> potal;

        Architecture architecture() const
        {
            return architecture_of(isa_revision);
        }

        uint64_t spad_rows() const noexcept
        {
            return uint64_t(spad_banks) * spad_bank_rows;
        }

        uint32_t acc_bank_rows() const noexcept
        {
            return acc_banks ? acc_rows / acc_banks : 0;
        }

        uint64_t spad_row_bytes() const noexcept
        {
            return (uint64_t(dim) * input_bits + 7) / 8;
        }

        uint64_t acc_row_bytes() const noexcept
        {
            return (uint64_t(dim) * accumulator_bits + 7) / 8;
        }

        uint64_t spad_capacity_bytes() const noexcept
        {
            return spad_rows() * spad_row_bytes();
        }

        uint64_t acc_capacity_bytes() const noexcept
        {
            return uint64_t(acc_rows) * acc_row_bytes();
        }

        static bool is_power_of_two(uint64_t value) noexcept
        {
            return value != 0 &&
                   (value & (value - 1)) == 0;
        }

        void validate() const
        {
            const auto arch = architecture();

            if (input_bits == 0 || input_bits > 32 ||
                array_output_bits == 0 || array_output_bits > 64 ||
                accumulator_bits == 0 || accumulator_bits > 64 ||
                accumulator_bits % 8 != 0 ||
                array_output_bits > accumulator_bits)
            {
                throw std::invalid_argument(
                    "Invalid integer precision geometry");
            }

            if (dim < 2 ||
                dim > 65535 ||
                !is_power_of_two(dim))
            {
                throw std::invalid_argument(
                    "Invalid array dimension");
            }

            if ((uint64_t(dim) * input_bits) % 8 != 0)
            {
                throw std::invalid_argument(
                    "Packed SPAD row must be whole bytes");
            }

            if (spad_banks < 2 ||
                !is_power_of_two(spad_banks) ||
                !is_power_of_two(spad_bank_rows))
            {
                throw std::invalid_argument(
                    "Invalid scratchpad geometry");
            }

            if (acc_banks < 2 ||
                !is_power_of_two(acc_banks) ||
                !is_power_of_two(acc_rows) ||
                acc_rows % acc_banks != 0 ||
                !is_power_of_two(acc_bank_rows()))
            {
                throw std::invalid_argument(
                    "Invalid accumulator geometry");
            }

            if (spad_bank_rows < dim ||
                acc_bank_rows() < dim)
            {
                throw std::invalid_argument(
                    "Bank cannot hold a full tile");
            }

            // 32-bit local address:
            // 6 metadata bits + 1 garbage bit.
            constexpr uint64_t max_local_rows =
                1ULL << 25;

            if (spad_rows() > max_local_rows ||
                acc_rows > max_local_rows)
            {
                throw std::invalid_argument(
                    "Local address width overflow");
            }

            if (!is_power_of_two(dma_max_bytes) ||
                dma_bus_bits == 0 ||
                dma_bus_bits % 8 != 0 ||
                !is_power_of_two(dma_bus_bits / 8) ||
                dma_max_bytes < dma_bus_bits / 8)
            {
                throw std::invalid_argument(
                    "Invalid DMA geometry");
            }

            if (arch == Architecture::PoTAL)
            {
                if (!potal ||
                    potal->scale_entries_per_column == 0 ||
                    potal->index_entries_per_buffer == 0)
                {
                    throw std::invalid_argument(
                        "Missing PoTAL metadata geometry");
                }
            }
            else if (potal)
            {
                throw std::invalid_argument(
                    "PoTAL metadata on Gemmini configuration");
            }
        }
    };

} // namespace togsim::npu