<?php

class BenchmarkStrconv {
  public function benchmarkStrvalInt32small() {
    return strval(217);
  }

  public function benchmarkStrvalInt32() {
    return strval(21745130);
  }

  public function benchmarkStrvalInt64() {
    return strval(214748364701);
  }
}
