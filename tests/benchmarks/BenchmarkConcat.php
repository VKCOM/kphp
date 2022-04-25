<?php

class BenchmarkConcat {
  private $s_short1 = 'hello, ';
  private $s_short2 = 'world!';
  private $s_long = '';
  private $s_empty = '';

  public function __construct() {
    $this->s_long = str_repeat('some text ', 100);
  }

  public function benchmarkEmptyEmpty() {
    return $this->s_empty . $this->s_empty;
  }

  public function benchmarkEmptyShort() {
    return $this->s_empty . $this->s_short1;
  }

  public function benchmarkEmptyLong() {
    return $this->s_empty . $this->s_long;
  }

  public function benchmarkShortShort() {
    return $this->s_short1 . $this->s_short2;
  }

  public function benchmarkShortConst() {
    return $this->s_short1 . 'suffix';
  }

  public function benchmarkShortShortShort() {
    return $this->s_short1 . $this->s_short2 . $this->s_short1;
  }

  public function benchmarkShortConstShort() {
    return $this->s_short1 . '==' . $this->s_short2;
  }

  public function benchmarkLongLong() {
    return $this->s_long . $this->s_long;
  }
}
