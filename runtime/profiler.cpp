#include "runtime/profiler.h"

#include "PHP/php-engine-vars.h"

#include "runtime/datetime.h"
#include "runtime/kphp_core.h"

var f$kphp_profiler_handle_call(string , double , double) __attribute__((weak));
var f$kphp_profiler_handle_call(string , double , double) {
  return var();
}

var f$kphp_profiler_finalize() __attribute__((weak));
var f$kphp_profiler_finalize() {
  return var();
}


std::vector <std::string> Profiler::function_name;
std::vector <double> Profiler::in_time;
std::vector <double> Profiler::calls_time;
int Profiler::profiler_status = -1;

Profiler::Profiler(const char* s){
  if (profiler_status == 1) {
  	return;
  }
  profiler_status = 0;
  function_name.push_back(s);
  in_time.push_back(microtime());
  calls_time.push_back(0);
}

Profiler::~Profiler() {
  if (profiler_status == 1) {
  	return;
  }
  php_assert(profiler_status != -1);
  profiler_status = 1;
  double start = in_time.back();
  php_assert(!function_name.empty());
  f$kphp_profiler_handle_call(string(function_name.back().c_str(), function_name.back().size()),
                               microtime() - start,
                               calls_time.back());
  function_name.pop_back();
  in_time.pop_back();
  calls_time.pop_back();
  if (!calls_time.empty()) {
    calls_time.back() += microtime() - start;
  } 
  profiler_status = 0;
} 

bool Profiler::is_enabled(){
  return profiler_status != -1;
}

void Profiler::finalize(){
  profiler_status = 1;
  f$kphp_profiler_finalize();
  profiler_status = 0;
}
