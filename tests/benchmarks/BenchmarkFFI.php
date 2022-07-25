<?php

class BenchmarkFFI {
  /** @var ffi_scope<glibc> */
  private $lib;

  /** @var ffi_cdata<C, double[]> */
  private $c_doubles;

  public function __construct() {
    $this->lib = FFI::cdef('
      #define FFI_SCOPE "glibc"

      // @kphp-ffi-signalsafe
      double sin(double);
    ');
    $this->c_doubles = FFI::new('double[4]');
    ffi_array_set($this->c_doubles, 0, 1.0);
  }

  private function inliningTest() {
    // this method should be inlineable even though it uses FFI ops
    // see #585
    return $this->lib->sin(ffi_array_get($this->c_doubles, 0));
  }

  public function benchmarkSin() {
    return $this->lib->sin(3.5);
  }

  public function benchmarkInlining() {
    return $this->inliningTest();
  }
}
