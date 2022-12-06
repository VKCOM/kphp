<?php

class BenchmarkConcat {
  private $s_short1 = 'hello, ';
  private $s_short2 = 'world!';
  private $s_long = '';
  private $s_empty = '';
  /** @var mixed[] */
  private $short_str_parts = [
    'string 1',
    'another short string',
    'https://github.com/VKCOM/kphp',
    'Big Brother is Watching You',
    'The choice for mankind lies between freedom and happiness and for the great bulk of mankind, happiness is better',
    'Who controls the past controls the future',
    'Who controls the present controls the past',
    'Reality exists in the human mind, and nowhere else',
  ];

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

  public function benchmarkAppend2() {
    $s = 'abc';
    $s .= $this->short_str_parts[0] . $this->short_str_parts[1];
    return $s;
  }

  public function benchmarkAppend4() {
    $s = 'abc';
    $s .= $this->short_str_parts[0] . $this->short_str_parts[1] . $this->short_str_parts[2] . $this->short_str_parts[3];
    return $s;
  }

  public function benchmarkAppend8() {
    $s = 'abc';
    $s .= $this->short_str_parts[0] .
          $this->short_str_parts[1] .
          $this->short_str_parts[2] .
          $this->short_str_parts[3] .
          $this->short_str_parts[4] .
          $this->short_str_parts[5] .
          $this->short_str_parts[6] .
          $this->short_str_parts[7];
    return $s;
  }

  public function benchmarkAppend4empty() {
    $s = '';
    $s .= $this->short_str_parts[0] . $this->short_str_parts[1] . $this->short_str_parts[2] . $this->short_str_parts[3];
    return $s;
  }

  public function benchmarkAppend4accum() {
    $s = 'abc';
    for ($i = 0; $i < 4; $i++) {
      $s .= $this->short_str_parts[0] . $this->short_str_parts[1] . $this->short_str_parts[2] . $this->short_str_parts[3];
    }
    return $s;
  }

  public function benchmarkAppend4complex() {
    $a = ['abc'];
    $a[0] .= $this->short_str_parts[0] . $this->short_str_parts[1] . $this->short_str_parts[2] . $this->short_str_parts[3];
    return $a[0];
  }

  public function benchmarkAppend4optional() {
    /** @var ?string $s */
    $s = null;
    $s .= $this->short_str_parts[0] . $this->short_str_parts[1] . $this->short_str_parts[2] . $this->short_str_parts[3];
    return $s;
  }

  public function benchmarkAppend4InterpolatedString() {
    $part = $this->short_str_parts[0];
    $s = '';
    $s .= "$part/$part/$part/$part";
    $s .= "$part/$part/$part/$part";
    return $s;
  }

  public function benchmarkAppend4InterpolatedInt() {
    $part = 2432;
    $s = '';
    $s .= "$part/$part/$part/$part";
    $s .= "$part/$part/$part/$part";
    return $s;
  }
}
