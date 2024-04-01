// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/pipes/calc-actual-edges.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"
#include "compiler/utils/idmap.h"

namespace filter_detail {
using EdgeInfo = CalcActualCallsEdgesPass::EdgeInfo;
using FunctionAndEdges = std::pair<FunctionPtr, std::vector<EdgeInfo>>;
} // namespace filter_detail

// Input: FunctionAndEdges (which function calls which).
// Performs:
// 1) Assigns FunctionData::id - it's done here as we know functions count at this point
// 2) Calculates can_throw: functions that call functions that may throw outside of the try block
//    are recorded as throwing themselves.
// 3) Calculates functions with empty bodies: functions that call non-empty functions are not empty.
// 4) Sends actually reachable functions to the os.
//    All instance functions are parsed, but this pipe sends only those that are actually used.
// 5) Removes the unused class methods.
class FilterOnlyActuallyUsedFunctionsF final : public SyncPipeF<filter_detail::FunctionAndEdges, FunctionPtr> {
public:
  using EdgeInfo = filter_detail::EdgeInfo;
  using FunctionAndEdges = filter_detail::FunctionAndEdges;

public:
  FilterOnlyActuallyUsedFunctionsF() = default;

  void on_finish(DataStream<FunctionPtr> &os) final;
};
