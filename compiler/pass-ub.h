#pragma once

#include "compiler/data_ptr.h"

class CalcFuncDepPass;

class CalcBadVars;

void fix_undefined_behaviour(FunctionPtr function);

#include "pass-ub.hpp"
