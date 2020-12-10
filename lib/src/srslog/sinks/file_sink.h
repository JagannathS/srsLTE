/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2020 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#ifndef SRSLOG_FILE_SINK_H
#define SRSLOG_FILE_SINK_H

#include "file_utils.h"
#include "srslte/srslog/sink.h"

namespace srslog {

/// This sink implementation writes to files. Includes the optional feature of
/// file rotation: a new file is created when file size exceeds an established
/// threshold.
class file_sink : public sink
{
public:
  file_sink(std::string name,
            size_t max_size,
            std::unique_ptr<log_formatter> f) :
    sink(std::move(f)),
    base_filename(std::move(name)),
    max_size((max_size == 0) ? 0 : std::max<size_t>(max_size, 4 * 1024))
  {}

  file_sink(const file_sink& other) = delete;
  file_sink& operator=(const file_sink& other) = delete;

  detail::error_string write(detail::memory_buffer buffer) override
  {
    // Create a new file the first time we hit this method.
    if (is_first_write()) {
      assert(!handler && "No handler should be created yet");
      if (auto err_str = create_file()) {
        return err_str;
      }
    }

    // Do not bother doing any work when the file was closed on a previous
    // error.
    if (!handler) {
      return {};
    }

    if (auto err_str = handle_rotation(buffer.size())) {
      return err_str;
    }

    return handler.write(buffer);
  }

  detail::error_string flush() override { return handler.flush(); }

protected:
  /// Returns the current file index.
  uint32_t get_file_index() const { return file_index; }

private:
  /// Returns true when the sink has never written data to a file, otherwise
  /// returns false.
  bool is_first_write() const { return file_index == 0; }

  /// Creates a new file and increments the file index counter.
  detail::error_string create_file()
  {
    return handler.create(
        file_utils::build_filename_with_index(base_filename, file_index++));
  }

  /// Handles the file rotation feature when it is activated.
  /// NOTE: The file handler must be valid.
  detail::error_string handle_rotation(size_t size)
  {
    assert(handler && "Expected a valid file handle");
    current_size += size;
    if (max_size && current_size >= max_size) {
      current_size = size;
      return create_file();
    }
    return {};
  }

private:
  const size_t max_size;
  const std::string base_filename;
  file_utils::file handler;
  size_t current_size = 0;
  uint32_t file_index = 0;
};

} // namespace srslog

#endif // SRSLOG_FILE_SINK_H
