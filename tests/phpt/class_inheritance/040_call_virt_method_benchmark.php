@ok k2_skip
<?php
# It's auto-generated file, please use `print_large_hierarchy.py`

class A_depth_0 {
  function run(int $x): int { return $x; }
}

class A_depth_1 extends A_depth_0
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_1_0 extends A_depth_1
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_1_1 extends A_depth_1
{
  function run(int $x): int { return $x; }
}

class A_depth_2 extends A_depth_1
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_2_0 extends A_depth_2
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_2_1 extends A_depth_2
{
  function run(int $x): int { return $x; }
}

class A_depth_3 extends A_depth_2
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_3_0 extends A_depth_3
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_3_1 extends A_depth_3
{
  function run(int $x): int { return $x; }
}

class A_depth_4 extends A_depth_3
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_4_0 extends A_depth_4
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_4_1 extends A_depth_4
{
  function run(int $x): int { return $x; }
}

class A_depth_5 extends A_depth_4
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_5_0 extends A_depth_5
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_5_1 extends A_depth_5
{
  function run(int $x): int { return $x; }
}

class A_depth_6 extends A_depth_5
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_6_0 extends A_depth_6
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_6_1 extends A_depth_6
{
  function run(int $x): int { return $x; }
}

class A_depth_7 extends A_depth_6
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_7_0 extends A_depth_7
{
  function run(int $x): int { return $x; }
}

class A_depth_temp_7_1 extends A_depth_7
{
  function run(int $x): int { return $x; }
}

function run_false_A_depth() {
  /**@var A_depth_0*/
  $x = false ? new A_depth_7() : new A_depth_0();
  $y = 1;
  $start = microtime(true);
  for ($i = 0; $i < 1000000; ++ $i) $y ^= $x->run(42);
  var_dump(!defined('kphp') ? true : (microtime(true) - $start) < 0.06);
}

run_false_A_depth();
function run_true_A_depth() {
  /**@var A_depth_0*/
  $x = true ? new A_depth_7() : new A_depth_0();
  $y = 1;
  $start = microtime(true);
  for ($i = 0; $i < 1000000; ++ $i) $y ^= $x->run(42);
  var_dump(!defined('kphp') ? true : (microtime(true) - $start) < 0.06);
}

run_true_A_depth();
class A_breadth_0 {
  function run(int $x): int { return $x; }
}

class A_breadth_1 extends A_breadth_0
{
  function run(int $x): int { return $x; }
}

class A_breadth_2 extends A_breadth_0
{
  function run(int $x): int { return $x; }
}

class A_breadth_3 extends A_breadth_0
{
  function run(int $x): int { return $x; }
}

class A_breadth_4 extends A_breadth_0
{
  function run(int $x): int { return $x; }
}

class A_breadth_5 extends A_breadth_0
{
  function run(int $x): int { return $x; }
}

class A_breadth_6 extends A_breadth_0
{
  function run(int $x): int { return $x; }
}

class A_breadth_7 extends A_breadth_0
{
  function run(int $x): int { return $x; }
}

function run_false_A_breadth() {
  /**@var A_breadth_0*/
  $x = false ? new A_breadth_7() : new A_breadth_0();
  $y = 1;
  $start = microtime(true);
  for ($i = 0; $i < 1000000; ++ $i) $y ^= $x->run(42);
  var_dump(!defined('kphp') ? true : (microtime(true) - $start) < 0.06);
}

run_false_A_breadth();
function run_true_A_breadth() {
  /**@var A_breadth_0*/
  $x = true ? new A_breadth_7() : new A_breadth_0();
  $y = 1;
  $start = microtime(true);
  for ($i = 0; $i < 1000000; ++ $i) $y ^= $x->run(42);
  var_dump(!defined('kphp') ? true : (microtime(true) - $start) < 0.06);
}

run_true_A_breadth();
