<?php

class BenchmarkExplode {
  private $words2 = 'hello world';
  private $words4 = 'one two three four';
  private $words8 = 'one two three four five six seven eight';
  private $delim = ' ';

  public function benchmarkExplodeToArray() {
    $res1 = explode(' ', $this->words2);
    $res2 = explode($this->delim, $this->words4);
    return count($res1) + count($res2);
  }

  public function benchmarkExplodeNth() {
    $x = explode(' ', $this->words2)[0];
    $y = explode($this->delim, $this->words4)[1];
    return strlen($x) + strlen($y);
  }

  public function benchmarkExplode1() {
    [$x] = explode(' ', $this->words2);
    [$y] = explode($this->delim, $this->words4);
    return strlen($x) + strlen($y);
  }

  public function benchmarkExplode2() {
    [$x1, $x2] = explode(' ', $this->words2);
    [$y1, $y2] = explode($this->delim, $this->words4);
    return strlen($x1) + strlen($x2) + strlen($y1) + strlen($y2);
  }

  public function benchmarkExplode3() {
    [$x1, $x2, $x3] = explode(' ', $this->words4);
    [$y1, $y2, $y3] = explode($this->delim, $this->words8);
    return strlen($x1) + strlen($x2) + strlen($x3) + strlen($y1) + strlen($y2) + strlen($y3);
  }

  public function benchmarkExplode4() {
    [$x1, $x2, $x3, $x4] = explode(' ', $this->words4);
    [$y1, $y2, $y3, $y4] = explode($this->delim, $this->words8);
    return strlen($x1) + strlen($x2) + strlen($x3) + strlen($x4) + strlen($y1) + strlen($y2) + strlen($y3) + strlen($y4);
  }

  public function benchmarkExplode3discard() {
    [1 => $x2] = explode(' ', $this->words4);
    [, $y2, $y3] = explode($this->delim, $this->words8);
    return strlen($x2) + strlen($y2) + strlen($y3);
  }

  public function benchmarkExplode2limit() {
    [$x1, $x2] = explode(' ', $this->words2, 2);
    [$y1, $y2] = explode($this->delim, $this->words4, 10);
    return strlen($x1) + strlen($x2) + strlen($y1) + strlen($y2);
  }

  public function benchmarkExplode3limit() {
    [$x1, $x2, $x3] = explode(' ', $this->words4, 3);
    [$y1, $y2, $y3] = explode($this->delim, $this->words8, 10);
    return strlen($x1) + strlen($x2) + strlen($x3) + strlen($y1) + strlen($y2) + strlen($y3);
  }

  public function benchmarkExplode4limit() {
    [$x1, $x2, $x3, $x4] = explode(' ', $this->words4, 4);
    [$y1, $y2, $y3, $y4] = explode($this->delim, $this->words8, 10);
    return strlen($x1) + strlen($x2) + strlen($x3) + strlen($x4) + strlen($y1) + strlen($y2) + strlen($y3) + strlen($y4);
  }
}
