#pragma once

#include <cstdint>
#include <stdexcept>

namespace togsim::npu
{

    enum class Architecture : uint8_t
    {
        Gemmini,
        PoTAL
    };

    // Simulator-defined ISA contract identifiers.
    // These are not official hardware version numbers.
    enum class IsaRevision : uint32_t
    {
        Unspecified   = 0,
        GemminiAisaV1 = 1, // gemmini-aisa @ 185293e
        // Simulator-defined PoTAL behavior; chipyard-aisa @ fab3d262
        // potal/ and potalw4/ are reference profiles, not separate contracts.
        PoTALAisaV1   = 2
    };

    inline Architecture architecture_of(IsaRevision revision)
    {
        switch (revision)
        {
        case IsaRevision::GemminiAisaV1:
            return Architecture::Gemmini;

        case IsaRevision::PoTALAisaV1:
            return Architecture::PoTAL;

        case IsaRevision::Unspecified:
            break;
        }

        throw std::invalid_argument(
            "Missing or unknown ISA revision");
    }

    namespace isa
    {
        constexpr uint8_t CUSTOM_OPCODE = 0x7b;

        enum class Funct : uint8_t
        {
            // Common base ISA.
            Config               = 0,
            Mvin2                = 1,
            Mvin                 = 2,
            Mvout                = 3,
            ComputePreloaded     = 4,
            ComputeAccumulate    = 5,
            Preload              = 6,
            Flush                = 7,

            LoopWs               = 8,
            LoopWsBounds         = 9,
            LoopWsAddrsAB        = 10,
            LoopWsAddrsDC        = 11,
            LoopWsStridesAB      = 12,
            LoopWsStridesDC      = 13,

            Mvin3                = 14,

            LoopConvWs           = 15,
            LoopConvConfig1      = 16,
            LoopConvConfig2      = 17,
            LoopConvConfig3      = 18,
            LoopConvConfig4      = 19,
            LoopConvConfig5      = 20,
            LoopConvConfig6      = 21,

            ClockGateEnable      = 22,

            // PoTAL extensions.
            ScaleWrite           = 23,
            ScaleRelease         = 24,
            AcceleratorFence     = 25,

            IndexWrite           = 26,
            IndexConfig          = 27,

            ResidentView         = 28,
            LdbOnly              = 29,
            Stream               = 30,

            // Experimental PoTAL index queue.
            IndexQueueHeader     = 32,
            IndexQueueBegin      = 33,
            IndexQueuePush       = 34,
            IndexQueueSeal       = 35,
            IndexQueueApply      = 36,
            IndexQueueGuardFence = 37,

            Counter              = 126
        };

        enum class ConfigKind : uint8_t
        {
            Execute = 0,
            Load = 1,
            Store = 2,
            Normalization = 3
        };

        inline bool is_base_funct(Funct funct) noexcept
        {
            const auto value = static_cast<uint8_t>(funct);

            return value <= 22 || value == 126;
        }

        inline bool is_potal_extension(Funct funct) noexcept
        {
            const auto value = static_cast<uint8_t>(funct);

            return (value >= 23 && value <= 30) ||
                   (value >= 32 && value <= 37);
        }

        inline bool is_index_queue_funct(Funct funct) noexcept
        {
            const auto value = static_cast<uint8_t>(funct);

            return value >= 32 && value <= 37;
        }
    } // namespace isa
} // namespace togsim::npu