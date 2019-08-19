<?php
require_once "init_typed_rpc_tests.php";
set_tl_mode(SAFE_NEW_TL_MODE);

use VK\TL\Functions\graph;
use VK\TL\Functions\text;
use VK\TL\Functions;

function getTypeWithFieldsMask($fields_mask) {
  echo "##### getTypeWithFieldsMask #####\n";
  $q = new graph\getTypeWithFieldsMask;
  $q->fields_mask = $fields_mask;
  $res = graph\getTypeWithFieldsMask::result(rpc_query_sync($q, GRAPH_PORT));
  echo $res->int1, " ", $res->int2, " ", $res->str;
  echo "\n";//################\n"
}

function storeTypeWithFieldsMask($fields_mask, $int1, $int2, $str) {
  echo "##### storeTypeWithFieldsMask #####\n";
  $q = new graph\storeTypeWithFieldsMask;
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

  $res = graph\storeTypeWithFieldsMask::result(rpc_query_sync($q, GRAPH_PORT));
  echo $res;
  echo "\n";//################\n"
}

function eitherIdentity($version, $str, $owner_id, $object_id, $type) {
  echo "##### eitherIdentity #####\n";
  $q = new graph\eitherIdentity;
  if ($version == 0) {
    $q->either = new VK\TL\Types\left__string__graph_Vertex;
    $t1 = $q->either;
    if ($t1 instanceof VK\TL\Types\left__string__graph_Vertex) {
      $t1->value = $str;
    }
  } else {
    $q->either = new VK\TL\Types\right__string__graph_Vertex;
    $t1 = $q->either;
    if ($t1 instanceof VK\TL\Types\right__string__graph_Vertex) {
      $t1->value = new VK\TL\Types\graph\vertex;
      $t1->value->owner_id = $owner_id;
      $t1->value->object_id = $object_id;
      $t1->value->type = $type;
    }
  }
  $res = graph\eitherIdentity::result(rpc_query_sync($q, GRAPH_PORT));
  if ($res instanceof \VK\TL\Types\left__string__graph_Vertex) {
    echo "left value: ", $res->value;
  } else if ($res instanceof \VK\TL\Types\right__string__graph_Vertex) {
    echo "right value: ", $res->value->owner_id, " ", $res->value->object_id, " ", $res->value->type;
  } else {
    echo "\n!!! SMTH WRONG !!!\n";
  }
  echo "\n";//################\n"
}

function tuple_test() {
  echo "##### tuple_test #####\n";
  $q = new \VK\TL\Functions\text\getExtraMask;
  $res = \VK\TL\Functions\text\getExtraMask::result(rpc_query_sync($q, TEXT_PORT));
  var_dump($res);
  echo "\n";//################\n"
}

function maybe_int_test() {
  echo "##### maybe_int_test #####\n";
  $q1 = new text\setSecret;
  $q1->uid = 42;
  $q1->s = "Hello!!!";
  $res_set = text\setSecret::result(rpc_query_sync($q1, TEXT_PORT));
  echo "set: ", $res_set, "\n";
  $q2 = new text\getSecret;
  $q2->uid = 42;
  $res_42 = text\getSecret::result(rpc_query_sync($q2, TEXT_PORT));
  if ($res_42) {
    echo "res 42: ", $res_42, "\n";
  } else {
    echo "FAIL\n";
  }
  $q2->uid = 43;
  $res_43 = text\getSecret::result(rpc_query_sync($q2, TEXT_PORT));
  if (!$res_43) {
    echo "res_43: Nothing\n";
  } else {
    echo "FAIL\n";
  }
  echo "\n";//################\n"
}

function array_with_flat_test() {
  echo "##### array_with_flat_test #####\n";
  $q = new VK\TL\Functions\retArray;
  $res = VK\TL\Functions\retArray::result(rpc_query_sync($q, GRAPH_PORT));
  echo "n = ", $res->n, "\n";
  echo "data = ", "\n";
  foreach ($res->data as $item) {
    echo $item->fun, " ", $item->rate_type, " ", $item->weight, "\n";
  }
  echo "\n";//################\n"
}

function array_without_flat_test() {
  echo "##### array_without_flat_test #####\n";
  $q = new Functions\retNotFlatArray;
  $res = Functions\retNotFlatArray::result(rpc_query_sync($q, GRAPH_PORT));
  echo "m = ", $res->m, "\n";
  echo "data = ", "\n";
  foreach ($res->data as $item) {
    echo $item->rate_type, " ", $item->max_value, " ", $item->min_value, "\n";
  }
  echo "\n";//################\n"
}

function vector_int_test() {
  echo "##### vector_int_test #####\n";
  $q = new text\onlineFriendsId;
  $q->uid = 42;
  $res = text\onlineFriendsId::result(rpc_query_sync($q, TEXT_PORT));
  if ($res) {
    var_dump($res);
  } else {
    echo "Nothing";
  }
  echo "\n";//################\n"
}

function flat_engine_stat_test() {
  echo "##### flat_engine_stat_test #####\n";
  $q = new VK\TL\Functions\engine\stat();
  $res = VK\TL\Functions\engine\stat::result(rpc_query_sync($q, GRAPH_PORT));
  var_dump($res);
  echo "\n";//################\n"
}

function flat2_test() {
  echo "##### flat2_test #####\n";
  $q = new VK\TL\Functions\graph\getFlat2();
  $res = VK\TL\Functions\graph\getFlat2::result(rpc_query_sync($q, GRAPH_PORT));
  echo "key = ", $res->key, "\n";
  echo "data = ";
  var_dump($res->value);
  echo "\n";//################\n"
}

eitherIdentity(0, "hello", 1, 2, 3);
eitherIdentity(1, "hello", 1, 2, 3);
array_with_flat_test();
array_without_flat_test();
tuple_test();
//echo "\ngetTypeWithFieldsMask(7)\n\n";
getTypeWithFieldsMask(7);
//echo "\ngetTypeWithFieldsMask(1)\n\n";
getTypeWithFieldsMask(1);
//echo "\ngetTypeWithFieldsMask(3)\n\n";
getTypeWithFieldsMask(3);
//echo "\nstoreTypeWithFieldsMask(5, 42, -1, 'qwerty')\n\n";
storeTypeWithFieldsMask(5, 42, -1, "qwerty");
//echo "\nstoreTypeWithFieldsMask(4, 42, -1, 'qwerty')\n\n";
storeTypeWithFieldsMask(4, 42, -1, "qwerty");
maybe_int_test();
vector_int_test();
flat2_test();
//flat_engine_stat_test();