@ok
<?php

class A {
  public $a = 0;
  public $arr = [];
  public $arr2 = [];
  static public $SA = '3';
}

A::$SA = '4';
$a = new A;
$a->a = 3;
$a->arr = [1,2,3];
$a->arr2 = [1,2,3];
$a->arr2[] = 'str';
