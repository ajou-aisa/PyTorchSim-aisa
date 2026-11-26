#include "npu/MemoryPort.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace togsim::npu
{
  MemoryPort::MemoryPort(std::size_t capacity)
      : _capacity(capacity)
  {
    if (capacity == 0)
    {
      throw std::invalid_argument(
          "MemoryPort capacity must be positive");
    }
  }

  std::optional<uint64_t> MemoryPort::try_submit(
      const MemoryRequest &request)
  {

    if (request.size == 0)
    {
      throw std::invalid_argument(
          "Invalid memory request size");
    }

    if (request.address >
        std::numeric_limits<uint64_t>::max() -
            (request.size - 1))
    {
      throw std::invalid_argument(
          "Memory address overflow");
    }

    if (request.op == MemoryOp::Read)
    {
      if (!request.data.empty())
      {
        throw std::invalid_argument(
            "Read request must not contain data");
      }
    }
    else if (request.op == MemoryOp::Write)
    {
      if (request.data.size() != request.size)
      {
        throw std::invalid_argument(
            "Invalid write payload size");
      }
    }
    else
    {
      throw std::invalid_argument(
          "Invalid memory operation");
    }

    if (_pending.size() >= _capacity)
    {
      return std::nullopt;
    }

    if (_next_id == 0)
    {
      throw std::overflow_error(
          "Memory request ID exhausted");
    }

    const uint64_t id = _next_id++;

    _pending.emplace(
        id,
        Pending{
            request.op,
            request.size,
            State::Queued});

    try
    {
      _requests.push_back({id, request});
    }
    catch (...)
    {
      _pending.erase(id);
      throw;
    }

    return id;
  }

  std::optional<IssuedMemoryRequest>
  MemoryPort::take_request()
  {

    if (_requests.empty())
    {
      return std::nullopt;
    }

    auto request = std::move(_requests.front());

    _requests.pop_front();

    _pending.at(request.id).state = State::InFlight;

    return request;
  }

  void MemoryPort::complete(MemoryResponse response)
  {
    auto it = _pending.find(response.id);

    if (it == _pending.end())
    {
      throw std::invalid_argument(
          "Unknown memory response ID");
    }

    auto &pending = it->second;

    if (pending.state != State::InFlight)
    {
      throw std::logic_error(
          "Invalid or duplicate memory completion");
    }

    if (response.status != MemoryStatus::Ok &&
        response.status != MemoryStatus::Error)
    {
      throw std::invalid_argument(
          "Invalid memory response status");
    }

    std::size_t expected_size = 0;

    if (response.status == MemoryStatus::Ok &&
        pending.op == MemoryOp::Read)
    {
      expected_size = pending.size;
    }

    if (response.data.size() != expected_size)
    {
      throw std::invalid_argument(
          "Invalid response payload size");
    }

    _responses.push_back(std::move(response));

    pending.state = State::Completed;
  }

  std::optional<MemoryResponse>
  MemoryPort::poll_response()
  {

    if (_responses.empty())
    {
      return std::nullopt;
    }

    auto response = std::move(_responses.front());

    _responses.pop_front();

    _pending.erase(response.id);

    return response;
  }

  bool MemoryPort::idle() const
  {
    return _pending.empty();
  }

  std::size_t MemoryPort::outstanding() const
  {
    return _pending.size();
  }

} // namespace togsim::npu