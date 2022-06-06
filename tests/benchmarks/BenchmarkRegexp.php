<?php

class BenchmarkRegexp {
  private $words = 'hello, world!';
  private $numbers = '87532352 384328 34';
  private $many_numbers = '4834 18 12 8128391 128 192 812 83 192 94';

  /** @var mixed */
  private $mixed_arr = [1, 'b', 3.5];

  public function benchmarkStringEmpty() {
    preg_match('/\w+/', $this->words, $m);
    return count($m);
  }

  public function benchmarkStringEmptyWithCaptures() {
    preg_match('/(\d+) (\d+) (\d+)/', $this->numbers, $m);
    return count($m);
  }

  public function benchmarkStringString() {
    $string_arr = ['a', 'b'];
    preg_match('/\w+/', $this->words, $string_arr);
    return count($string_arr);
  }

  public function benchmarkStringStringWithCaptures() {
    $string_arr = ['a', 'b'];
    preg_match('/(\d+) (\d+) (\d+)/', $this->numbers, $string_arr);
    return count($string_arr);
  }

  public function benchmarkStringArrayOfMixed() {
    $array_of_mixed = [1, 'a'];
    preg_match('/\w+/', $this->words, $array_of_mixed);
    return count($this->mixed_arr);
  }

  public function benchmarkStringMixed() {
    preg_match('/\w+/', $this->words, $this->mixed_arr);
    return count($this->mixed_arr);
  }

  public function benchmarkStringMixedWithCaptures() {
    preg_match('/(\d+) (\d+) (\d+)/', $this->numbers, $this->mixed_arr);
    return count($this->mixed_arr);
  }

  public function benchmarkOffsetCapture() {
    preg_match('/(\d+) (\d+) (\d+)/', $this->numbers, $m, PREG_OFFSET_CAPTURE);
    return count($m);
  }

  public function benchmarkAllStringEmpty() {
    preg_match_all('/\w+/', $this->words, $m);
    return count($m);
  }

  public function benchmarkAllStringEmptyWithCaptures() {
    preg_match_all('/(\d+) (\d+) (\d+)/', $this->many_numbers, $m);
    return count($m);
  }

  public function benchmarkAllStringString() {
    $string_arr = ['a', 'b'];
    preg_match_all('/\w+/', $this->words, $string_arr);
    return count($string_arr);
  }

  public function benchmarkAllStringStringWithCaptures() {
    $string_arr = ['a', 'b'];
    preg_match_all('/(\d+) (\d+) (\d+)/', $this->many_numbers, $string_arr);
    return count($string_arr);
  }

  public function benchmarkAllStringArrayOfMixed() {
    $array_of_mixed = [1, 'a'];
    preg_match_all('/\w+/', $this->words, $array_of_mixed);
    return count($this->mixed_arr);
  }

  public function benchmarkAllStringMixed() {
    preg_match_all('/\w+/', $this->words, $this->mixed_arr);
    return count($this->mixed_arr);
  }

  public function benchmarkAllStringMixedWithCaptures() {
    preg_match_all('/(\d+) (\d+) (\d+)/', $this->many_numbers, $this->mixed_arr);
    return count($this->mixed_arr);
  }

  public function benchmarkAllOffsetCapture() {
    preg_match_all('/(\d+) (\d+) (\d+)/', $this->many_numbers, $m, PREG_OFFSET_CAPTURE);
    return count($m);
  }

  public function benchmarkVarPattern() {
    $pat = '/' . '\w+' . '/';
    preg_match_all($pat, $this->words, $this->mixed_arr);
    return count($this->mixed_arr);
  }
}
