<?php

class BenchmarkMultiSwitch {
  private $string_value = 'string';
  private $int_value = 10;

  public function benchmarkMultiString() {
    $result = 0;
    switch ($this->string_value) {
    case 'hello':
      $result += 1;
      break;
    case 'world':
      $result += 2;
      break;
    case 'string':
      $result += 3;
      break;
    }
    switch ($this->string_value) {
    case $this->getStringValue():
      $result *= 10;
      break;
    case $this->getStringValue2():
      $result *= 20;
      break;
    case 'hello':
      $result *= 30;
      break;
    }
    return $result;
  }

  public function benchmarkMultiInt() {
    $result = 0;
    switch ($this->int_value) {
    case 54:
      $result += 1;
      break;
    case 603:
      $result += 2;
      break;
    case 10:
      $result += 3;
      break;
    }
    switch ($this->int_value) {
    case $this->getIntValue():
      $result *= 10;
      break;
    case ord('x'):
      $result *= 20;
      break;
    case $this->int_value + 1:
      $result *= 30;
      break;
    }
    return $result;
  }

  private function getStringValue() { return $this->string_value; }
  private function getStringValue2() { return 'ok'; }

  private function getIntValue() { return $this->int_value; }
}
