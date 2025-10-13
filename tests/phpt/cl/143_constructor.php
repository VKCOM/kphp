@ok
<?php

class ClassConstructorBeforeMembers {
  public function __construct() { }

  public $x_int = 1;
  public $x_str = "hello";
}

class ClassConstructorWithSimpleReturn {
  public function __construct() {
    return;
  }

  public $x_int = 1;
  public $x_str = "hello";
}

class ClassConstructorWithSimpleReturnThis {
  public function __construct() {
    return $this;
  }

  public $x_int = 1;
  public $x_str = "hello";
}

class ClassConstructorWithIfReturn {
  public function __construct() {
    if ($this->x_int > 2) {
      $this->x_int = 18;
      return;
    }
    if ($this->x_int < 0) {
      $this->x_str = "world";
      return $this;
    }
  }

  public $x_int = 1;
  public $x_str = "hello";
}

function test_constructor_before_members() {
  $x = new ClassConstructorBeforeMembers();
  var_dump($x->x_int);
  var_dump($x->x_str);
}

function test_constructor_with_simple_return() {
  $x = new ClassConstructorWithSimpleReturn();
  var_dump($x->x_int);
  var_dump($x->x_str);
}

function test_constructor_with_simple_return_this() {
  $x = new ClassConstructorWithSimpleReturnThis();
  var_dump($x->x_int);
  var_dump($x->x_str);
}

function test_constructor_with_if_return() {
  $x = new ClassConstructorWithIfReturn();
  var_dump($x->x_int);
  var_dump($x->x_str);
}

test_constructor_before_members();
test_constructor_with_simple_return();
test_constructor_with_simple_return_this();
test_constructor_with_if_return();
