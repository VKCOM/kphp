<?php

namespace ComplexScenario;

class Stats {
  /** @var int[] */
  public $int_stats = [];

  /** @var float[] */
  public $float_stats = [];

  /** @var string[] */
  public $string_stats = [];

  function add_stat(string $key, string $value) {
    if (is_numeric($value)) {
      if ((int)$value == $value && strpos($value, ".") === false) {
        $this->int_stats[$key] = (int)$value;
      } else {
        $this->float_stats[$key] = (float)$value;
      }
    } else {
      $this->string_stats[$key] = (string)$value;
    }
  }

  function normalize() {
    ksort($this->int_stats);
    ksort($this->float_stats);
    ksort($this->string_stats);
  }
}
