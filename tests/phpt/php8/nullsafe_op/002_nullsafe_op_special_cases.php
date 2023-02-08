@ok php8
<?php

require_once 'kphp_tester_include.php';

class A {
  public int $data;
  function __construct() {
      $this->data = 42;
  }
  function NULL() {
      var_dump('A::NULL()');
      if (2 > 1) {
          return $this;
      }
      return NULL;
  }

  function self() {
      var_dump('A::self()');
      return $this;
  }

  function data() {
      var_dump('A::data()');
      return $this->data;
  }

  function data_param(int $tmp) {
      var_dump('A::data_param(int)');
      return $this->data;
  }
}

function getA() {
  return new A;
}

function condA(int $val) {
    if ($val % 2 == 1) {
        return new A();
    }
    return NULL;
}

/// Start testing
/** @var A[] $arr */
$arr = array(NULL, getA(), new A, NULL);
var_dump($arr[0]?->data);
var_dump($arr[1]?->data());
var_dump($arr[2]?->data);
var_dump($arr[3]?->data());

$a = condA(0);
var_dump(($a === NULL ? getA() : condA(1))?->data);
var_dump(($a === NULL ? getA() : condA(0))?->data);
$a = condA(1);
var_dump(($a === NULL ? getA() : condA(1))?->data);
var_dump(($a === NULL ? getA() : condA(0))?->data);
