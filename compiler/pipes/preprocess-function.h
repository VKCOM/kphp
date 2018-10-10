#pragma once

#include "compiler/bicycle.h"
#include "compiler/data_ptr.h"

/*** Replace __FUNCTION__ ***/
/*** Set function_id for all function calls ***/
class PreprocessFunctionF {
  using InstanceOfFunctionTemplatePtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<FunctionPtr, InstanceOfFunctionTemplatePtr>;
public:
  void execute(FunctionPtr function, OStreamT &os);
};
