#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

namespace togsim::npu
{
    enum class MemoryOp : uint8_t
    {
        Read,
        Write
    };

    enum class MemoryStatus : uint8_t
    {
        Ok,
        Error
    };

    struct MemoryRequest
    {
        MemoryOp op;
        uint64_t address;
        uint32_t size;

        // Empty for reads.
        // Contains exactly size bytes for writes.
        std::vector<uint8_t> data;
    };

    struct IssuedMemoryRequest
    {
        uint64_t id;
        MemoryRequest request;
    };

    struct MemoryResponse
    {
        uint64_t id;
        MemoryStatus status;

        // Read data on successful read.
        // Empty for writes and errors.
        std::vector<uint8_t> data;
    };

    class MemoryPort
    {
    public:
        explicit MemoryPort(std::size_t capacity);

        MemoryPort(const MemoryPort &) = delete;
        MemoryPort &operator=(const MemoryPort &) = delete;

        // Called by the NPU.
        std::optional<uint64_t> try_submit(
            const MemoryRequest &request);

        // Called by the memory backend.
        std::optional<IssuedMemoryRequest> take_request();

        // Called when the memory operation completes.
        void complete(MemoryResponse response);

        // Called by the NPU.
        std::optional<MemoryResponse> poll_response();

        bool idle() const;

        std::size_t outstanding() const;

    private:
        enum class State : uint8_t
        {
            Queued,
            InFlight,
            Completed
        };

        struct Pending
        {
            MemoryOp op;
            uint32_t size;
            State state;
        };

        const std::size_t _capacity;

        uint64_t _next_id = 1;

        std::deque<IssuedMemoryRequest> _requests;

        std::deque<MemoryResponse> _responses;

        std::unordered_map<uint64_t, Pending> _pending;
    };

} // namespace togsim::npu