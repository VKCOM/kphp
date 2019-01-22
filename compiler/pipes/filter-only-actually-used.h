#pragma once

#include "compiler/pipes/calc-actual-edges.h"
#include "compiler/threading/data-stream.h"
#include "compiler/utils/idmap.h"

/**
 * Имеет на входе FunctionAndEdges — какая функция какие вызывает —  делает следующее:
 * 1) присваивает FunctionData::id — это делается именно тут, когда известно количество функций
 * 2) вычисляет can_throw: функции, которые вызывают те, что могут кидать исключения не внутри try — сами могут кидать
 * 3) вычисляет функции с пустым телом: функции, которые вызывают не пустые функции - сами не пустые
 * 4) в os отправляет только реально достижимые (так, инстанс-функции парсятся все, но дальше пойдут только вызываемые)
 * 5) удаляет неиспользуемые методы классов
 */
class FilterOnlyActuallyUsedFunctionsF {
public:
  using EdgeInfo = CalcActualCallsEdgesPass::EdgeInfo;
  using FunctionAndEdges = std::pair<FunctionPtr, std::vector<EdgeInfo>>;
private:
  DataStream<FunctionAndEdges> tmp_stream;

public:
  FilterOnlyActuallyUsedFunctionsF();

  void execute(FunctionAndEdges f, DataStream<FunctionPtr> &);

  void on_finish(DataStream<FunctionPtr> &os);
};
