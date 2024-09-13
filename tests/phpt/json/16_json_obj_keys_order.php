@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {
  public string $a;
  public string $b;
  public string $c;
}

function test_direct_order() {
  $json = "{\"a\":\"aaa\",\"b\":\"bbb\",\"c\":\"ccc\"}";
  $obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($obj));
}

function test_mismatch_order() {
  $jsons = [
    "{\"a\":\"aaa\",\"c\":\"ccc\",\"b\":\"bbb\"}",
    "{\"b\":\"bbb\",\"a\":\"aaa\",\"c\":\"ccc\"}",
    "{\"b\":\"bbb\",\"c\":\"ccc\",\"a\":\"aaa\"}",
    "{\"c\":\"ccc\",\"b\":\"bbb\",\"a\":\"aaa\"}",
    "{\"c\":\"ccc\",\"a\":\"aaa\",\"b\":\"bbb\"}"
  ];

  foreach ($jsons as $json) {
    $obj = JsonEncoder::decode($json, "A");
    var_dump(to_array_debug($obj));
  }
}

function test_excessive_json_keys() {
  $json = "{\"foo\":\"foo\",\"a\":\"aaa\",\"b\":\"bbb\",\"bar\":\"bar\",\"c\":\"ccc\",\"baz\":\"baz\"}";
  $obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($obj));
}

test_direct_order();
test_mismatch_order();
test_excessive_json_keys();
