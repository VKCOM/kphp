<?php

use VK\TL\_common\Types\left__string__graph_Vertex;
use VK\TL\_common\Types\right__string__graph_Vertex;
use VK\TL\graph\Functions\graph_retArray;
use VK\TL\graph\Functions\graph_retMaybeInt;
use VK\TL\graph\Functions\graph_retMaybeVectorInt;
use VK\TL\graph\Functions\graph_retNotFlatArray;
use VK\TL\graph\Functions\graph_retTuple;
use VK\TL\_common\Functions\rpcDestActor;
use VK\TL\_common\Functions\rpcDestFlags;
use VK\TL\_common\Types\rpcInvokeReqExtra;
use VK\TL\engine\Functions\engine_nop;
use VK\TL\engine\Functions\engine_stat;
use VK\TL\graph\Functions\graph_eitherIdentity;
use VK\TL\graph\Functions\graph_getFlat2;
use VK\TL\graph\Functions\graph_getTypeWithFieldsMask;
use VK\TL\graph\Functions\graph_storeTypeWithFieldsMask;
use VK\TL\graph\Types\graph_typeWithFieldsMask;
use VK\TL\graph\Types\graph_vertex;
use VK\TL\memcache\Functions\memcache_get;
use VK\TL\memcache\Functions\memcache_set;
use VK\TL\memcache\Types\memcache_strvalue;
use VK\TL\rpcProxy\Functions\rpcProxy_diagonal;
use VK\TL\text\Functions\text_getExtraMask;
use VK\TL\text\Functions\text_getSecret;
use VK\TL\text\Functions\text_onlineFriendsId;
use VK\TL\text\Functions\text_setSecret;

require_once "init_typed_rpc_tests.php";

$FailedAsserts = 0;

function assertEq($expected, $actual)
{
  global $FailedAsserts;
  if ($expected != $actual) {
    $bt = debug_backtrace();
    $function = $bt[1]['function'];
    $line = $bt[0]['line'];
    echo "ASSERT EQ FAILED! In $function:$line\n";
    echo "Expected:\n";
    var_dump($expected);
    echo "Actual:\n";
    var_dump($actual);
    $FailedAsserts++;
  }
}

function assertFalse() {
  assertEq(1, 2);
}

function getTypeWithFieldsMask($fields_mask)
{
  echo "##### getTypeWithFieldsMask fields_mask=$fields_mask #####\n";
  $q = new graph_getTypeWithFieldsMask($fields_mask);
  $res = graph_getTypeWithFieldsMask::result(rpc_query_sync($q, MEMCACHE_PORT));
  if ($fields_mask & 1) {
    assertEq(42, $res->int1);
  }
  if ($fields_mask & 2) {
    assertEq(43, $res->int2);
  }
  if ($fields_mask & 4) {
    assertEq("Hello", $res->str);
  }
}

function storeTypeWithFieldsMask($fields_mask, $int1, $int2, $str)
{
  echo "##### storeTypeWithFieldsMask fields_mask=$fields_mask int1=$int1 int2=$int2 str=$str #####\n";
  $q = new graph_storeTypeWithFieldsMask();
  $q->m = $fields_mask;
  $q->x = new graph_typeWithFieldsMask();
  if ($fields_mask & (1 << 0)) {
    $q->x->int1 = $int1;
  }
  if ($fields_mask & (1 << 1)) {
    $q->x->int2 = $int2;
  }
  if ($fields_mask & (1 << 2)) {
    $q->x->str = $str;
  }

  $res = graph_storeTypeWithFieldsMask::result(rpc_query_sync($q, MEMCACHE_PORT));
  assertEq(
    boolval($fields_mask & 1) * $int1 +
    boolval($fields_mask & 2) * $int2 +
    boolval($fields_mask & 4) * strlen($str),
    $res);
}

function eitherIdentity($version, $str, $owner_id, $object_id, $type)
{
  echo "##### eitherIdentity version=$version str=$str owner_id=$owner_id object_id=$object_id type=$type #####\n";
  $q = new graph_eitherIdentity();
  if ($version == 0) {
    $q->either = new left__string__graph_Vertex();
    $t1 = $q->either;
    if ($t1 instanceof left__string__graph_Vertex) {
      $t1->value = $str;
    }
  } else {
    $q->either = new VK\TL\_common\Types\right__string__graph_Vertex;
    $t1 = $q->either;
    if ($t1 instanceof VK\TL\_common\Types\right__string__graph_Vertex) {
      $t1->value = new graph_vertex();
      $t1->value->owner_id = $owner_id;
      $t1->value->object_id = $object_id;
      $t1->value->type = $type;
    }
  }

  $res = graph_eitherIdentity::result(rpc_query_sync($q, MEMCACHE_PORT));
  if ($res instanceof left__string__graph_Vertex) {
    assertEq("hello", $res->value);
  } else if ($res instanceof right__string__graph_Vertex) {
    assertEq(1, $res->value->owner_id);
    assertEq(2, $res->value->object_id);
    assertEq(3, $res->value->type);
  } else {
    assertEq(NAN, NAN);
  }
}

function tuple_test($x, $len)
{
  echo "##### tuple_test: x=$x len=$len #####\n";
  $q = new graph_retTuple($x, $len);
  $res = graph_retTuple::result(rpc_query_sync($q, MEMCACHE_PORT));
  assertEq($len, count($res));
  foreach ($res as $item) {
    assertEq($x, $item);
  }
}

function maybe_int_test($x)
{
  echo "##### maybe_int_test: x=$x #####\n";
  $q = new graph_retMaybeInt($x);
  $res = graph_retMaybeInt::result(rpc_query_sync($q, MEMCACHE_PORT));
  if ($res) {
    assertEq($x, $res);
  } else {
    assertEq(0, $x);
  }
}

function array_with_flat_test()
{
  echo "##### array_with_flat_test #####\n";
  $q = new graph_retArray();
  $res = graph_retArray::result(rpc_query_sync($q, MEMCACHE_PORT));
  assertEq(4, $res->n);
  assertEq(4, count($res->data));
  $i = 0;
  foreach ($res->data as $item) {
    assertEq(200 + $i, $item->fun);
    assertEq(100 + $i, $item->rate_type);
    assertEq($i, $item->weight);
    $i++;
  }
}

function array_without_flat_test()
{
  echo "##### array_without_flat_test #####\n";
  $q = new graph_retNotFlatArray;
  $res = graph_retNotFlatArray::result(rpc_query_sync($q, MEMCACHE_PORT));
  assertEq(4, $res->m);
  assertEq(4, count($res->data));
  $i = 0;
  foreach ($res->data as $item) {
    assertEq($i, $item->rate_type);
    assertEq(200 + $i, $item->max_value);
    assertEq(100 + $i, $item->min_value);
    $i++;
  }
}

function maybe_vector_int_test($x, $len)
{
  echo "##### maybe_vector_int_test x=$x len=$len #####\n";
  $q = new graph_retMaybeVectorInt($x, $len);
  $res = graph_retMaybeVectorInt::result(rpc_query_sync($q, MEMCACHE_PORT));
  if ($res) {
    assert(count($res) === $len);
    foreach ($res as $item) {
      assert($item === $x);
    }
  }
}

function flat_engine_stat_test()
{
  echo "##### flat_engine_stat_test #####\n";
  $q = new engine_stat();
  $res = engine_stat::result(rpc_query_sync($q, MEMCACHE_PORT));
  assertEq("0", $res["rpc_sent_queries"]);
  assertEq("0", $res["rpc_sent_errors"]);
  assertEq("0", $res["rpc_answers_received"]);
  assertEq("0", $res["rpc_answers_error"]);
}

function flat2_test()
{
  echo "##### flat2_test #####\n";
  $q = new graph_getFlat2();
  $res = graph_getFlat2::result(rpc_query_sync($q, MEMCACHE_PORT));
  assertEq(100, $res->key);
  assertEq($res->value, [101, 43]);
}

function memcache_test(string $key, string $value)
{
  echo "##### flat2_test key=$key value=$value #####\n";
  $q = new memcache_set($key, 0, 0, $value);
  $res = memcache_set::result(rpc_query_sync($q, MEMCACHE_PORT));
  assertEq(true, $res);
  $q = new memcache_get($key);
  $res = memcache_get::result(rpc_query_sync($q, MEMCACHE_PORT));
  if ($res instanceof memcache_strvalue) {
    assertEq($value, $res->value);
  } else {
    assertEq(NAN, NAN);
  }
}

function rpc_proxy_diagonal_storeTypeWithFieldsMask($fields_mask, $int1, $int2, $str)
{
  echo "##### rpc_proxy_diagonal_storeTypeWithFieldsMask fields_mask=$fields_mask int1=$int1 int2=$int2 str=$str #####\n";
  $q = new graph_storeTypeWithFieldsMask;
  $q->m = $fields_mask;
  $q->x = new graph_typeWithFieldsMask;
  if ($fields_mask & (1 << 0)) {
    $q->x->int1 = $int1;
  }
  if ($fields_mask & (1 << 1)) {
    $q->x->int2 = $int2;
  }
  if ($fields_mask & (1 << 2)) {
    $q->x->str = $str;
  }
  $diagonalQ = new rpcProxy_diagonal;
  $diagonalQ->query = $q;
  $res = rpcProxy_diagonal::result(rpc_query_sync($diagonalQ, PROXY_PORT, 1));

  assertEq(1, count($res));

  foreach ($res as $v) {
    $sub_res = graph_storeTypeWithFieldsMask::functionReturnValue($v);
    assertEq(
      boolval($fields_mask & 1) * $int1 +
      boolval($fields_mask & 2) * $int2 +
      boolval($fields_mask & 4) * strlen($str),
      $sub_res);
  }
}

function rpc_proxy_diagonal_getTypeWithFieldsMask($fields_mask)
{
  echo "##### rpc_proxy_diagonal_getTypeWithFieldsMask fields_mask=$fields_mask #####\n";
  $q = new graph_getTypeWithFieldsMask;
  $q->fields_mask = $fields_mask;
  $diagonalQ = new rpcProxy_diagonal;
  $diagonalQ->query = $q;
  $arr = rpcProxy_diagonal::result(rpc_query_sync($diagonalQ, PROXY_PORT, 1));
  foreach ($arr as $item) {
    $v = graph_getTypeWithFieldsMask::functionReturnValue($item);
    if ($fields_mask & 1) {
      assertEq(42, $v->int1);
    }
    if ($fields_mask & 2) {
      assertEq(43, $v->int2);
    }
    if ($fields_mask & 4) {
      assertEq("Hello", $v->str);
    }
  }
}

function rpc_proxy_diagonal_engine_nop()
{
  echo "##### rpc_proxy_diagonal_engine_nop #####\n";
  $q = new engine_nop();
  $diagonalQ = new rpcProxy_diagonal;
  $diagonalQ->query = $q;
  $arr = rpcProxy_diagonal::result(rpc_query_sync($diagonalQ, 4444, 1));
  foreach ($arr as $item) {
    $v = engine_nop::functionReturnValue($item);
    assertEq(true, $v);
  }
}

function result_multi_excl_test()
{
  echo "##### result_multi_excl_test #####\n";
  // send_and_vardump(['_' => 'rpcDestFlags', 'flags' => 3 + (1 << 16), 'extra' => ['_' => 'rpcInvokeReqExtra', 'wait_binlog_pos' => 36], 'query' => $q], 6666);
  $q = new engine_nop;
  $excl_q = new rpcDestFlags(3, new rpcInvokeReqExtra(), $q);
  $excl_excl_q = new rpcDestActor(0, $excl_q);

  $excl_excl_resp = rpc_query_sync($excl_excl_q, MEMCACHE_PORT);
  assertEq("VK\\TL\\engine\\Functions\\engine_nop_result", get_class($excl_excl_resp->getResult()));

//  $excl_res = rpcDestActor::result($excl_excl_resp);
//  echo "1: ", get_class($excl_res), "\n";
//
//  $res = rpcDestFlags::functionReturnValue($excl_res);
//  echo "2: ", get_class($res), "\n";

//  $value = engine_nop::functionReturnValue($res);
  $value = engine_nop::functionReturnValue($excl_excl_resp->getResult());
  assertEq(true, $value);
}

//\rpcDestActor::result(false);
//\rpcDestFlags::result(false);

function value_multi_excl_test()
{
  echo "##### value_multi_excl_test #####\n";
  $q = new engine_nop;
  $excl_q = new rpcDestActor(0, $q);
  $excl_excl_q = new rpcDestActor(0, $excl_q);
  $excl_excl_excl_q = new rpcProxy_diagonal($excl_excl_q);

  $excl_excl_excl_resp = rpc_query_sync($excl_excl_excl_q, PROXY_PORT, 1);
  assertEq("VK\\TL\\rpcProxy\\Functions\\rpcProxy_diagonal_result", get_class($excl_excl_excl_resp->getResult()));

  $excl_excl_res = rpcProxy_diagonal::result($excl_excl_excl_resp);

  foreach ($excl_excl_res as $item) {
    assertEq("VK\\TL\\engine\\Functions\\engine_nop_result", get_class($item));


//    $excl_res = rpcDestActor::functionReturnValue($item);
//    echo "2: ", get_class($excl_res), "\n";
//
//    $res = rpcDestActor::functionReturnValue($excl_res);
//    echo "3: ", get_class($res), "\n";

//    $value = engine_nop::functionReturnValue($res);
    $value = engine_nop::functionReturnValue($item);
    assertEq(true, $value);
  }
}

function generic_fm_test($maybe_y = null)
{
  echo "##### generic_fm_test maybe_y=$maybe_y #####\n";
  $obj = new \VK\TL\_common\Types\testGenericFm__int();
  $obj->a = [1,2,3];
  $obj->x = 42;
  $obj->y = $maybe_y;
  $resp = rpc_query_sync(new \VK\TL\_common\Functions\test_geneirc_fm($obj->calculateN(true), $obj), MEMCACHE_PORT);
  if ($resp->isError()) {
    echo $resp->getError()->error . "\n";
    assertFalse();
    return;
  }
  $res = \VK\TL\_common\Functions\test_geneirc_fm::result($resp);
  assertEq($obj->a, $res->a);
  assertEq($obj->x, $res->x);
  assertEq($obj->y, $res->y);
}

function test_rpc_invoke_req_extra_headers($key, $value, $ignore_result, $use_actor) {
  echo "##### test_rpc_invoke_req_extra_headers key=$key, ignore_result=$ignore_result, use_actor=$use_actor #####\n";

  if ($use_actor) {
    $conn = new_rpc_connection("127.0.0.1", PROXY_PORT, 1);
  } else {
    $conn = new_rpc_connection("127.0.0.1", MEMCACHE_PORT);
  }

  // @write memcache.set key:string flags:int delay:int value:string = Bool;
  $set_query = ["_" => "memcache.set", "key" => $key, "value" => $value, "flags" => 0, "delay" => 0];
  $set_ids = rpc_tl_query($conn, [$set_query], 1000, $ignore_result);
  if (!$ignore_result) {
    assertEq(true, rpc_tl_query_result($set_ids)[0]["result"]);
  }
  rpc_flush();
  for ($_ = 0; $_ < 100000; $_++) {} // hang

  $get_query = ["_" => "memcache.get", "key" => $key];
  $id = rpc_tl_query_one($conn, $get_query);
  $res = rpc_tl_query_result_one($id);

  assertEq($value, $res["result"]["value"]);
}

function main()
{
  echo "@@@@@@ TESTS STARTED @@@@@@\n";
  memcache_test("test-key", "test-value");

  getTypeWithFieldsMask(7);
  getTypeWithFieldsMask(1);
  getTypeWithFieldsMask(3);
  storeTypeWithFieldsMask(5, 42, -1, "qwerty");
  storeTypeWithFieldsMask(4, 42, -1, "qwerty");

  array_without_flat_test();
  tuple_test(2, 3);

  maybe_int_test(4);
  maybe_int_test(0);
  maybe_vector_int_test(1, 3);
  maybe_vector_int_test(0, 3);

  array_with_flat_test();
  flat2_test();
  flat_engine_stat_test();

  eitherIdentity(0, "hello", 1, 2, 3);
  eitherIdentity(1, "hello", 1, 2, 3);

  rpc_proxy_diagonal_storeTypeWithFieldsMask(7, 1, 2, "hello");
  rpc_proxy_diagonal_getTypeWithFieldsMask(7);
  rpc_proxy_diagonal_engine_nop();
  result_multi_excl_test();
  value_multi_excl_test();

  generic_fm_test();
  generic_fm_test(100);

  test_rpc_invoke_req_extra_headers("test1", "hello", false, true);
  test_rpc_invoke_req_extra_headers("test2", "hello", true, false);
  test_rpc_invoke_req_extra_headers("test3", "hello", true, true);

  global $FailedAsserts;
  echo "@@@@@@ TESTS ENDED @@@@@@\n";
  if ($FailedAsserts) {
    echo "\e[91m------ FAILED ------\e[0m\n";
  } else {
    echo "\e[32m------ SUCCESSFUL ------\e[0m\n";
  }
  echo "@@@@@@ Failed asserts: $FailedAsserts\n";
}

main();
