@kphp_should_fail
/Expected '}' after obj/
<?php

// This is invalid in both PHP and KPHP

class Example {
  public $prop = 10;
}

$obj = new Example();

var_dump("-${obj->prop}-");
