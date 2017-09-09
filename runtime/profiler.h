#pragma once

#include <string>
#include <vector>

class Profiler {
  static std::vector <std::string> function_name;
  static std::vector <double> in_time;
  static std::vector <double> calls_time;
  static int profiler_status;
 
 public:

  Profiler(const char* s); 
  ~Profiler(); 
  static bool is_enabled();
  static void finalize();
};