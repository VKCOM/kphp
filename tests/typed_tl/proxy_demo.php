<?php
require_once "init_typed_rpc_tests.php";
set_tl_mode(SAFE_NEW_TL_MODE);

use VK\TL\Functions\rpcDestActor;
use VK\TL\Functions\rpcProxy;
use VK\TL\Types;

function rpc_proxy_diagonal_storeTypeWithFieldsMask($fields_mask, $int1, $int2, $str) {
  echo "##### rpc_proxy_diagonal_storeTypeWithFieldsMask #####\n";
  $q = new VK\TL\Functions\graph\storeTypeWithFieldsMask;
  $q->m = $fields_mask;
  $q->x = new VK\TL\Types\graph\typeWithFieldsMask;
  if ($fields_mask & (1 << 0)) {
    $q->x->int1 = $int1;
  }
  if ($fields_mask & (1 << 1)) {
    $q->x->int2 = $int2;
  }
  if ($fields_mask & (1 << 2)) {
    $q->x->str = $str;
  }
  $diagonalQ = new rpcProxy\diagonal;
  $diagonalQ->query = $q;
  $res = rpcProxy\diagonal::result(rpc_query_sync($diagonalQ, 4444, 1));

  foreach ($res as $v) {
    $sub_res = VK\TL\Functions\graph\storeTypeWithFieldsMask::functionReturnValue($v);
    echo "result: ", $sub_res, "\n";
  }
  echo "\n";
}

function rpc_proxy_diagonal_getTypeWithFieldsMask($fields_mask) {
  echo "##### rpc_proxy_diagonal_getTypeWithFieldsMask #####\n";
  $q = new VK\TL\Functions\graph\getTypeWithFieldsMask;
  $q->fields_mask = $fields_mask;
  $diagonalQ = new rpcProxy\diagonal;
  $diagonalQ->query = $q;
  $arr = rpcProxy\diagonal::result(rpc_query_sync($diagonalQ, 4444, 1));
  foreach ($arr as $item) {
    $v = VK\TL\Functions\graph\getTypeWithFieldsMask::functionReturnValue($item);
    echo "type with fields mask: ", $v->int1, " ", $v->int2, " ", $v->str, "\n";
  }
  echo "\n";
}

function  rpc_proxy_diagonal_engine_nop() {
  echo "##### rpc_proxy_diagonal_engine_nop #####\n";
  $q = new VK\TL\Functions\engine\nop();
  $diagonalQ = new rpcProxy\diagonal;
  $diagonalQ->query = $q;
  $arr = rpcProxy\diagonal::result(rpc_query_sync($diagonalQ, 4444, 1));
  foreach ($arr as $item) {
    $v = VK\TL\Functions\engine\nop::functionReturnValue($item);
    echo "nop is ", $v;
  }
  echo "\n";
}

function result_multi_excl_test() {
  echo "##### result_multi_excl_test #####\n";
  // send_and_vardump(['_' => 'rpcDestFlags', 'flags' => 3 + (1 << 16), 'extra' => ['_' => 'rpcInvokeReqExtra', 'wait_binlog_pos' => 36], 'query' => $q], 6666);
  $q = new VK\TL\Functions\engine\nop;
  $excl_q = new VK\TL\Functions\rpcDestFlags(3 + (1 << 16), new Types\rpcInvokeReqExtra(36), $q);
  $excl_excl_q = new VK\TL\Functions\rpcDestActor(0, $excl_q);

  $excl_excl_resp = rpc_query_sync($excl_excl_q, 6666);
  echo "0: ", get_class($excl_excl_resp->getResult()), "\n";

//  $excl_res = VK\TL\Functions\rpcDestActor::result($excl_excl_resp);
//  echo "1: ", get_class($excl_res), "\n";
//
//  $res = VK\TL\Functions\rpcDestFlags::functionReturnValue($excl_res);
//  echo "2: ", get_class($res), "\n";

//  $value = VK\TL\Functions\engine\nop::functionReturnValue($res);
  $value = VK\TL\Functions\engine\nop::functionReturnValue($excl_excl_resp->getResult());
  echo "final nop res = ", $value;

  echo "\n";
}

//\VK\TL\Functions\rpcDestActor::result(false);
//\VK\TL\Functions\rpcDestFlags::result(false);

function value_multi_excl_test() {
  echo "##### value_multi_excl_test #####\n";
  $q = new VK\TL\Functions\engine\nop;
  $excl_q = new VK\TL\Functions\rpcDestActor(0, $q);
  $excl_excl_q = new VK\TL\Functions\rpcDestActor(0, $excl_q);
  $excl_excl_excl_q = new rpcProxy\diagonal($excl_excl_q);

  $excl_excl_excl_resp = rpc_query_sync($excl_excl_excl_q, 4444, 1);
  echo "0: ", get_class($excl_excl_excl_resp->getResult()), "\n";

  $excl_excl_res = rpcProxy\diagonal::result($excl_excl_excl_resp);

  foreach ($excl_excl_res as $item) {
    echo "1: ", get_class($item), "\n";

//    $excl_res = VK\TL\Functions\rpcDestActor::functionReturnValue($item);
//    echo "2: ", get_class($excl_res), "\n";
//
//    $res = VK\TL\Functions\rpcDestActor::functionReturnValue($excl_res);
//    echo "3: ", get_class($res), "\n";

//    $value = VK\TL\Functions\engine\nop::functionReturnValue($res);
    $value = VK\TL\Functions\engine\nop::functionReturnValue($item);
    echo "final nop res = ", $value;
  }
  echo "\n";
}

rpc_proxy_diagonal_storeTypeWithFieldsMask(7, 1, 2, "hello");
rpc_proxy_diagonal_getTypeWithFieldsMask(7);
rpc_proxy_diagonal_engine_nop();
result_multi_excl_test();
value_multi_excl_test();
