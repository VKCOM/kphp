<?php

class BenchmarkStrconv {
  private $i32small = 24;
  private $i32big = 48324546;
  /** @var mixed */
  private $mixed = 'hello';
  private $map = [];

  public function __construct() {
    $this->map = [
      $this->i32big => 'value',
    ];
  }

  public function benchmarkStrvalInt32small() {
    return strval(217);
  }

  public function benchmarkStrvalInt32() {
    return strval(21745130);
  }

  public function benchmarkStrvalInt64() {
    return strval(214748364701);
  }

  public function benchmarkConcatEmptyAndInt32small() {
    return '' . $this->i32small;
  }

  public function benchmarkConcatEmptyAndMixed() {
    return $this->mixed . '';
  }

  public function benchmarkStringBuildSingleInt32small() {
    return "$this->i32small";
  }

  public function benchmarkStringBuildSingleMixed() {
    return "$this->mixed";
  }

  public function benchmarkConcatOperand() {
    $i = 54543937;
    return (string)$i . $this->mixed;
  }
}
