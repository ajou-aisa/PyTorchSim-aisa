#include "npu/MemoryPort.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

using namespace togsim::npu;

static void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

int main() {
  MemoryPort port(2);

  std::array<uint8_t, 16> memory{};

  const MemoryRequest write{
      MemoryOp::Write,
      0,
      4,
      {10, 20, 30, 40}
  };

  const MemoryRequest read{
      MemoryOp::Read,
      0,
      4,
      {}
  };

  // Submit two requests.
  const auto write_id = port.try_submit(write);
  const auto read_id = port.try_submit(read);

  check(write_id.has_value(), "Write rejected");
  check(read_id.has_value(), "Read rejected");
  check(*write_id != *read_id, "Duplicate ID");

  // Outstanding capacity must be enforced.
  check(!port.try_submit(read), "Backpressure failed");

  check(!port.idle(), "Unexpected idle");
  check(port.outstanding() == 2,
        "Invalid outstanding count");

  // Backend takes both requests.
  const auto req1 = port.take_request();
  const auto req2 = port.take_request();

  check(req1 && req2, "Missing request");
  check(!port.take_request(), "Unexpected request");

  // Simulate write completion.
  std::copy(
      req1->request.data.begin(),
      req1->request.data.end(),
      memory.begin() + req1->request.address);

  port.complete({
      req1->id,
      MemoryStatus::Ok,
      {}
  });

  // Simulate read completion.
  const auto begin =
      memory.begin() + req2->request.address;

  port.complete({
      req2->id,
      MemoryStatus::Ok,
      std::vector<uint8_t>(
          begin,
          begin + req2->request.size)
  });

  // Responses still occupy outstanding entries.
  check(!port.idle(), "Premature idle");

  const auto ack = port.poll_response();

  check(ack && ack->id == *write_id,
        "Invalid write acknowledgment");

  const auto result = port.poll_response();

  check(result && result->id == *read_id,
        "Invalid read response");

  check(result->data == write.data,
        "Read/write payload mismatch");

  check(port.idle(), "MemoryPort did not drain");

  std::cout << "MEMORY_PORT_PASS\n";

  return 0;
}