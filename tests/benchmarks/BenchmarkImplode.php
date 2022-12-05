<?php

class BenchmarkImplode {
  /** @var string[] */
  private $vec0;
  /** @var string[] */
  private $vec1;
  /** @var string[] */
  private $vec3;
  /** @var string[] */
  private $vec16;
  /** @var string[] */
  private $vec128;
  /** @var string[] */
  private $vec1024;
  /** @var string[] */
  private $vec16_long;
  /** @var string[] */
  private $vec128_long;
  /** @var string[] */
  private $vec16_short;
  /** @var string[] */
  private $vec128_short;
  /** @var mixed[] */
  private $mixed_arr16;
  /** @var int[] */
  private $int_arr16;
  /** @var int[] */
  private $int_arr128;
  /** @var float[] */
  private $float_arr16;
  /** @var float[] */
  private $float_arr128;
  private $map3;
  /** @var string[] */
  private $map16;

  public function __construct() {
    $s = 'a string value';
    $this->vec0 = [];
    $this->vec1 = [$s];
    $this->vec3 = array_fill(0, 3, $s);
    $this->vec16 = array_fill(0, 16, $s);
    $this->vec128 = array_fill(0, 128, $s);
    $this->vec1024 = array_fill(0, 1024, $s);
    $long_s = str_repeat($s, 100);
    $this->vec16_long = array_fill(0, 16, $long_s);
    $this->vec128_long = array_fill(0, 128, $long_s);
    $short_s = 'a';
    $this->vec16_short = array_fill(0, 16, $short_s);
    $this->vec128_short = array_fill(0, 128, $short_s);
    /** @var mixed $mixed_s */
    $mixed_s = 'a string value';
    $this->mixed_arr16 = array_fill(0, 16, $mixed_s);
    for ($i = 0; $i < 16; $i++) {
      $this->int_arr16[] = $i + 1;
    }
    for ($i = 0; $i < 128; $i++) {
      $this->int_arr128[] = $i + 1;
    }
    for ($i = 0; $i < 16; $i++) {
      $this->float_arr16[] = (($i * $i) * 10.5) + 0.4335;
    }
    for ($i = 0; $i < 128; $i++) {
      $this->float_arr128[] = (($i * $i) * 10.5) + 0.4335;
    }
    for ($i = 0; $i < 3; $i++) {
      $this->map3["k$i"] = $s;
    }
    for ($i = 0; $i < 16; $i++) {
      $this->map16["k$i"] = $s;
    }
  }

  public function benchmarkEmptyArray() {
    return implode('--', $this->vec0);
  }

  public function benchmarkVector1() {
    return implode('--', $this->vec1);
  }

  public function benchmarkVector3() {
    return implode('--', $this->vec3);
  }

  public function benchmarkVector16() {
    return implode('--', $this->vec16);
  }

  public function benchmarkVector128() {
    return implode('--', $this->vec128);
  }

  public function benchmarkVector1024() {
    return implode('--', $this->vec1024);
  }

  public function benchmarkVector16long() {
    return implode('--', $this->vec16_long);
  }

  public function benchmarkVector128long() {
    return implode('--', $this->vec128_long);
  }

  public function benchmarkVector16short() {
    return implode('--', $this->vec16_short);
  }

  public function benchmarkVector128short() {
    return implode('--', $this->vec128_short);
  }
  
  public function benchmarkMap3() {
    return implode('--', $this->map3);
  }

  public function benchmarkMap16long() {
    return implode('--', $this->map16);
  }

  public function benchmarkMixedVector16() {
    return implode('--', $this->mixed_arr16);
  }

  public function benchmarkIntVector16() {
    return implode('--', $this->int_arr16);
  }

  public function benchmarkIntVector128() {
    return implode('--', $this->int_arr128);
  }

  public function benchmarkFloatVector16() {
    return implode('--', $this->float_arr16);
  }

  public function benchmarkFloatVector128() {
    return implode('--', $this->float_arr128);
  }

  public function benchmarkVector3emptyDelim() {
    return implode('', $this->vec3);
  }

  public function benchmarkVector16emptyDelim() {
    return implode('', $this->vec16);
  }

  public function benchmarkVector128emptyDelim() {
    return implode('', $this->vec128);
  }

  public function benchmarkVector1024emptyDelim() {
    return implode('', $this->vec1024);
  }

  public function benchmarkMixedVector16emptyDelim() {
    return implode('', $this->mixed_arr16);
  }
}
