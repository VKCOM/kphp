<?php

class BenchmarkArraySplice {
  private $shared_vec5 = [];
  private $shared_vec20 = [];
  private $shared_vec100 = [];

  public function __construct() {
    $this->shared_vec5 = array_fill(0, 5, 100);
    $this->shared_vec20 = array_fill(0, 20, 100);
    $this->shared_vec100 = array_fill(0, 100, 100);
  }

  public function benchmarkVector20RemoveMidShared() {
    $a = $this->shared_vec20;
    array_splice($a, 5, 10);
  }

  public function benchmarkVector20RemoveMidNodiscardShared() {
    $a = $this->shared_vec20;
    return array_splice($a, 5, 10);
  }

  public function benchmarkVectorReuse() {
    $a = array_fill(0, 20, 100); // preallocated 20 elems
    for ($i = 0; $i < 10; $i++) {
      array_splice($a, 0); // clear array
      for ($j < 0; $j < 10; ++$j) {
        $a[] = $j;
      }
    }
  }

  public function benchmarkVector5ReplaceTail1() {
    $a = array_fill(0, 5, 100);
    array_splice($a, count($a) - 1, count($a), [1, 2, 3]);
  }

  public function benchmarkVector5ReplaceTail4() {
    $a = array_fill(0, 5, 100);
    array_splice($a, 1, count($a), [1, 2, 3]);
  }

  public function benchmarkVector20ReplaceTail1() {
    $a = array_fill(0, 20, 100);
    array_splice($a, count($a) - 1, count($a), [1, 2, 3, 4, 5]);
  }

  public function benchmarkVector20ReplaceTail19() {
    $a = array_fill(0, 20, 100);
    array_splice($a, 1, count($a), [1, 2, 3, 4, 5]);
  }

  public function benchmarkVector100ReplaceTail1() {
    $a = array_fill(0, 100, 100);
    array_splice($a, count($a) - 1, count($a), [1, 2, 3, 4, 5, 6, 7, 8]);
  }

  public function benchmarkVector100ReplaceTail99() {
    $a = array_fill(0, 100, 100);
    array_splice($a, 1, count($a), [1, 2, 3, 4, 5, 6, 7, 8]);
  }

  public function benchmarkVector5Truncate1() {
    $a = array_fill(0, 5, 100);
    array_splice($a, count($a) - 1);
  }

  public function benchmarkVector5Truncate4() {
    $a = array_fill(0, 5, 100);
    array_splice($a, 1);
  }

  public function benchmarkVector20Truncate1() {
    $a = array_fill(0, 20, 100);
    array_splice($a, count($a) - 1);
  }

  public function benchmarkVector20Truncate19() {
    $a = array_fill(0, 20, 100);
    array_splice($a, 1);
  }

  public function benchmarkVector100Truncate1() {
    $a = array_fill(0, 100, 100);
    array_splice($a, count($a) - 1);
  }

  public function benchmarkVector100Truncate99() {
    $a = array_fill(0, 100, 100);
    array_splice($a, 1);
  }

  public function benchmarkVector5ReplaceTailShared1() {
    $a = $this->shared_vec5;
    array_splice($a, count($a) - 1, count($a), [1, 2, 3]);
  }

  public function benchmarkVector5ReplaceTailShared4() {
    $a = $this->shared_vec5;
    array_splice($a, 1, count($a), [1, 2, 3]);
  }

  public function benchmarkVector20ReplaceTailShared1() {
    $a = $this->shared_vec20;
    array_splice($a, count($a) - 1, count($a), [1, 2, 3, 4, 5]);
  }

  public function benchmarkVector20ReplaceTailShared19() {
    $a = $this->shared_vec20;
    array_splice($a, 1, count($a), [1, 2, 3, 4, 5]);
  }

  public function benchmarkVector100ReplaceTailShared1() {
    $a = $this->shared_vec100;
    array_splice($a, count($a) - 1, count($a), [1, 2, 3, 4, 5, 6, 7, 8]);
  }

  public function benchmarkVector100ReplaceTailShared99() {
    $a = $this->shared_vec100;
    array_splice($a, 1, count($a), [1, 2, 3, 4, 5, 6, 7, 8]);
  }

  public function benchmarkVector5TruncateShared1() {
    $a = $this->shared_vec5;
    array_splice($a, count($a) - 1);
  }

  public function benchmarkVector5TruncateShared4() {
    $a = $this->shared_vec5;
    array_splice($a, 1);
  }

  public function benchmarkVector20TruncateShared1() {
    $a = $this->shared_vec20;
    array_splice($a, count($a) - 1);
  }

  public function benchmarkVector20TruncateShared19() {
    $a = $this->shared_vec20;
    array_splice($a, 1);
  }

  public function benchmarkVector100TruncateShared1() {
    $a = $this->shared_vec100;
    array_splice($a, count($a) - 1);
  }

  public function benchmarkVector100TruncateShared99() {
    $a = $this->shared_vec100;
    array_splice($a, 1);
  }
}
