<?php

class BenchmarkImplode {
  private $parts1 = ['a single string'];
  private $parts3 = ['first string', 'second string', 'third string'];

  public function benchmarkImplode1() {
    return implode(' ', $this->parts1);
  }

  public function benchmarkImplode3() {
    return implode(' ', $this->parts3);
  }
}
