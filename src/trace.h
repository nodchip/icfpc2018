#pragma once

#include <deque>
#include <string>

#include "command.h"

typedef std::deque<Command> Trace;
bool output_trace(std::string output_path, const Trace& trace);
