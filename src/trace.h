#pragma once

#include <deque>
#include <string>

#include "command.h"

class Trace : public std::deque<Command> {
 public:
  bool output_trace(std::string output_path);
  bool output_trace_json(std::string input_path);
  bool input_trace(std::string input_path);
  void print_detailed();
  Trace transpose();
};
