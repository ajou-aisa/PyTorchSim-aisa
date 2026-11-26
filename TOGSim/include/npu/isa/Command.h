#pragma once

#include <cstdint>

namespace togsim::npu {
    struct RawCommand {
        uint64_t command_id = 0;
        uint32_t hart_id = 0;

        // Original RISC-V instruction word.
        uint32_t instruction = 0;

        // Source register values.
        uint64_t rs1 = 0;
        uint64_t rs2 = 0;

        uint8_t opcode() const noexcept {
            return instruction & 0x7f;
        }

        uint8_t funct7() const noexcept {
            return (instruction >> 25) & 0x7f;
        }

        uint8_t rd() const noexcept {
            return (instruction >> 7) & 0x1f;
        }

        uint8_t rs1_reg() const noexcept {
            return (instruction >> 15) & 0x1f;
        }

        uint8_t rs2_reg() const noexcept {
            return (instruction >> 20) & 0x1f;
        }
        
        bool xd() const noexcept {
            return (instruction >> 14) & 1;
        }

        bool xs1() const noexcept {
            return (instruction >> 13) & 1;
        }

        bool xs2() const noexcept {
            return (instruction >> 12) & 1;
        }
    };
}