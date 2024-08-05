@ok k2_skip
<?php

class A {
  public $x = 1;
}

class EmptyClass {
  public function __construct() {}
}

function static_vars() {
  static $null = null;
  static $boolean = true;
  $boolean = false;

  static $integer = 123;
  $integer = 452;

  static $double = 1.23;
  $double = 2.31;

  static $const_string = "hello world";
  static $const_array = [1, 2];

  static $dynamic_string = "foo";
  $dynamic_string .= "bar";

  static $dynamic_array = [1];
  $dynamic_array[] = 2;

  static $instance = null;
  $instance = new A;

  static $empty_instance = null;
  $empty_instance = new EmptyClass;
}

function global_vars() {
  global $null, $boolean, $integer, $double, $const_string, $const_array,
         $dynamic_string, $dynamic_array, $instance, $empty_instance;

  $null = null;
  $boolean = true;
  $integer = 123;
  $double = 1.23;
  $const_string = "hello world";
  $const_array = [1, 2];

  $dynamic_string = "foo";
  $dynamic_string .= "bar";

  $dynamic_array = [1];
  $dynamic_array[] = 2;

  $instance = new A;
  $empty_instance = new EmptyClass;
}

function class_static_vars() {
  class ClassWithStaticVars {
    static public $null = null;
    static public $boolean = false;
    static public $integer = 0;
    static public $double = 0.0;
    static public $const_string = "";
    static public $const_array = [];

    static public $dynamic_string = "";
    static public $dynamic_array = [];

    /** @var A */
    static public $instance = null;
    /** @var EmptyClass */
    static public $empty_instance = null;
  }

  ClassWithStaticVars::$null = null;
  ClassWithStaticVars::$boolean = true;
  ClassWithStaticVars::$integer = 123;
  ClassWithStaticVars::$double = 1.23;
  ClassWithStaticVars::$const_string = "hello world";
  ClassWithStaticVars::$const_array = [1, 2];

  ClassWithStaticVars::$dynamic_string = "foo";
  ClassWithStaticVars::$dynamic_string .= "bar";

  ClassWithStaticVars::$dynamic_array = [1];
  ClassWithStaticVars::$dynamic_array[] = 2;

  ClassWithStaticVars::$instance = new A;
  ClassWithStaticVars::$empty_instance = new EmptyClass;
}

function test_get_global_vars_memory_stats() {
#ifndef KPHP
  $greater_than_16 = [
    "\$dynamic_array" => 64,
    "static_vars::\$dynamic_array" => 64,
    "ClassWithStaticVars::\$dynamic_array" => 64,
    "ClassWithStaticVars::\$dynamic_string" => 19,
    "\$dynamic_string" => 19,
    "static_vars::\$dynamic_string" => 19
  ];

  $non_empty_vars = $greater_than_16;
  $non_empty_vars["\$instance"] = 16;
  $non_empty_vars["static_vars::\$instance"] = 16;
  $non_empty_vars["ClassWithStaticVars::\$instance"] = 16;

  $all_vars = $non_empty_vars;
  $all_vars["ClassWithStaticVars::\$const_array"] = 0;
  $all_vars["ClassWithStaticVars::\$const_string"] = 0;
  $all_vars["ClassWithStaticVars::\$empty_instance"] = 0;
  $all_vars["\$const_string"] = 0;
  $all_vars["static_vars::\$const_string"] = 0;
  $all_vars["\$const_array"] = 0;
  $all_vars["static_vars::\$const_array"] = 0;
  $all_vars["\$empty_instance"] = 0;
  $all_vars["static_vars::\$empty_instance"] = 0;

  ksort($non_empty_vars);
  ksort($all_vars);
  ksort($greater_than_16);
  var_dump($non_empty_vars);
  var_dump($non_empty_vars);
  var_dump($all_vars);
  var_dump($greater_than_16);
  return;
#endif
  $non_empty_vars = get_global_vars_memory_stats();
  ksort($non_empty_vars);
  var_dump($non_empty_vars);
  $non_empty_vars = get_global_vars_memory_stats(0);
  ksort($non_empty_vars);
  var_dump($non_empty_vars);
  $all_vars = get_global_vars_memory_stats(-1);
  ksort($all_vars);
  var_dump($all_vars);
  $greater_than_16 = get_global_vars_memory_stats(16);
  ksort($greater_than_16);
  var_dump($greater_than_16);
}

static_vars();
global_vars();
class_static_vars();

test_get_global_vars_memory_stats();
