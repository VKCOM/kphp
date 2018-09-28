#pragma once

#include "compiler/bicycle.h"
#include "compiler/pipes/function-and-edges.h"

/**
 * Имеет на входе FunctionAndEdges — какая функция какие вызывает —  делает следующее:
 * 1) присваивает FunctionData::id — это делается именно тут, когда известно количество функций
 * 2) вычисляет throws_flag: функции, которые вызывают те, что могут кидать исключения не внутри try — сами могут кидать
 * 3) в os отправляет только реально достижимые (так, инстанс-функции парсятся все, но дальше пойдут только вызываемые)
 */
class FilterOnlyActuallyUsedFunctionsF {
  DataStream<FunctionAndEdges> tmp_stream;
  int throws_func_cnt = 0;
  int actually_called_func_cnt = 0;

  void calc_throws_having_call_edges(vector<FunctionAndEdges> &all);

  void calc_throws_dfs(FunctionPtr from, IdMap<vector<FunctionPtr>> &graph);

  void calc_actually_used_having_call_edges(vector<FunctionAndEdges> &all, DataStream<FunctionPtr> &os);

  void calc_actually_used_dfs(FunctionPtr from, IdMap<vector<FunctionAndEdges::EdgeInfo>> &graph, IdMap<int> &used, DataStream<FunctionPtr> &os);

public:
  FilterOnlyActuallyUsedFunctionsF();

  void execute(FunctionAndEdges f, DataStream<FunctionPtr> &);

  void on_finish(DataStream<FunctionPtr> &os);
};
