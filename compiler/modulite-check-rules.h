// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/data/modulite-data.h"

void modulite_check_when_use_class(FunctionPtr usage_context, ClassPtr klass);
void modulite_check_when_use_global_const(FunctionPtr usage_context, DefinePtr used_c);
void modulite_check_when_use_constant(FunctionPtr usage_context, DefinePtr used_c, ClassPtr requested_class);
void modulite_check_when_call_function(FunctionPtr usage_context, FunctionPtr called_f);
void modulite_check_when_use_static_field(FunctionPtr usage_context, VarPtr property, ClassPtr requested_class);
void modulite_check_when_global_keyword(FunctionPtr usage_context, const std::string &global_var_name);
