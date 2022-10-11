<?php

class BenchmarkTmpString {
  /** @var mixed[] */
  private $assoc_arr = [
    'hello world' => 1,
    'foo' => 2,
    'bar' => 3,
  ];
  private $intstr = '392563_foo';
  private $str_with_spaces = '  hello, world';
  private $str_with_spaces2 = '  hello, world  ';
  private $str8 = '';
  private $str32 = '';

  public function __construct() {
    $this->str8 = str_repeat('x', 8);
    $this->str32 = str_repeat('x', 32);
  }

  public function benchmarkIntvalSubstr() {
    return (int)substr($this->intstr, 0, 6) + intval(substr($this->intstr, 0));
  }

  public function benchmarkAppendSubstr() {
    $s = '';
    $s .= substr($this->str32, 0, 20);
    $s .= substr($this->str8, 2);
    return $s;
  }

  public function benchmarkConcat2() {
    return substr($this->str32, 0, 20) . substr($this->str8, 2);
  }

  public function benchmarkConcat3() {
    return substr($this->str32, 0, 20) . substr($this->str8, 2) . trim($this->str_with_spaces2);
  }

  public function benchmarkConcatSubstrConv() {
    return (string)substr($this->str32, 2) . $this->str8;
  }

  public function benchmarkSubstr3() {
    $s = 'some example string';
    return substr(substr(substr($s, 1), 0, 6), 1, 4);
  }

  public function benchmarkIndexTrimSubstr() {
    $s = '!  hello world';
    return $this->assoc_arr[trim(substr($s, 1))];
  }

  public function benchmarkIndexSubstr() {
    $this->assoc_arr[substr($this->intstr, strlen('392563_'))] = 10;
    $this->assoc_arr[substr($this->intstr, strlen('392563_'))] += 2;
    $res1 = $this->assoc_arr[substr($this->intstr, strlen('392563_'))];
    $as_mixed = $this->assoc_arr;
    $res2 = $as_mixed[substr($this->intstr, strlen('392563_'))];
    return $res1 + $res2;
  }

  public function benchmarkTrimSubstrA() {
    return trim(substr($this->str_with_spaces, 1));
  }

  public function benchmarkTrimSubstrB() {
    return trim(substr($this->str_with_spaces2, 2));
  }

  public function benchmarkTrimSubstrC() {
    return trim((string)substr($this->str_with_spaces, 2));
  }

  public function benchmarkSubstrBoolContext() {
    return (bool)substr($this->intstr, 2);
  }

  public function benchmarkTrimBoolContext() {
    return trim($this->str_with_spaces) && trim($this->str_with_spaces2);
  }
}
