#pragma once

#include <deque>
#include <string>

#include "command.h"

class Trace : public std::deque<Command> {
 public:
  bool output_trace(std::string output_path);
  bool input_trace(std::string input_path);
};
