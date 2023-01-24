@ok php8

<?php

class C {
  public int $dd;
  function __construct() {
    $this->dd = 42;
  }
  function null() {
      var_dump('C::null()');
      if (2 > 1) {
        return $this;
      }
      return null;
  }

  function self() {
      var_dump('C::self()');
      return $this;
  }
  
  function d() {
    var_dump('C::d()');
    return $this->dd;
  }

  function d_param(int $tmp, int $tmp2, int $tmp3) {
    var_dump('C::d_param(int, int, int)');
    return $this->dd;
  }
}

class B {
  public C $cc;
  function __construct() {
    $this->cc = new C;
  }
  function null() {
      var_dump('B::null()');
      if (2 > 1) {
        return $this;
      }
      return null;
  }

  function self() {
      var_dump('B::self()');
      return $this;
  }
  
  function c() {
    var_dump('B::c()');
    return $this->cc;
  }

  function c_param(int $tmp, int $tmp2) {
    var_dump('B::c_param(int, int)');
    return $this->cc;
  }
}

class A {
  public B $bb;
  function __construct() {
    $this->bb = new B;
  }
  function null() {
      var_dump('A::null()');
      if (2 > 1) {
        return $this;
      }
      return null;
  }

  function self() {
      var_dump('A::self()');
      return $this;
  }
  
  function b() {
    var_dump('A::b()');
    return $this->bb;
  }

  function b_param(int $tmp) {
    var_dump('A::b_param(int)');
    return $this->bb;
  }

  public static B $b_static;
  public static function get_b_static() {
    return self::$b_static;
  }

}

function getA() {
  return new A;
}

/// Start testing

function field_chain(?A $a) {
  return $a?->bb?->cc?->dd;
}

function field_chain_starts_with_fun_call() {
  return getA()?->bb?->cc?->dd;
}

var_dump(field_chain(new A));
var_dump(field_chain(NULL));
var_dump(field_chain_starts_with_fun_call());

function method_chain(?A $a) {
  return $a?->self()?->b()?->self()?->c()?->self()?->d();
}

function method_chain_starts_with_fun_call() {
  return getA()?->self()?->b()?->self()?->c()?->self()?->dd;
}

var_dump(method_chain(new A));
var_dump(method_chain(NULL));
var_dump(method_chain_starts_with_fun_call());


function method_params_chain(?A $a) {
  return $a?->self()?->b_param(1)?->self()?->c_param(42, 146)?->self()?->d_param(42, 146, 42);
}

var_dump(method_params_chain(new A));
var_dump(method_params_chain(NULL));

function mixed_chain(?A $a) {
  return $a?->self()?->bb?->self()?->cc?->self()?->d();
}

function mixed_chain_starts_with_fun_call() {
  return getA()?->bb?->self()?->cc?->self()?->dd;
}

var_dump(mixed_chain(new A));
var_dump(mixed_chain(NULL));
var_dump(mixed_chain_starts_with_fun_call());

function null_chain_1(?A $a) {
  return $a?->null()?->bb?->self()?->cc?->self()?->d();
}

function null_chain_2(?A $a) {
  return $a?->self()?->b()?->null()?->c()?->self()?->d();
}

function null_chain_3(?A $a) {
  return $a?->self()?->bb?->self()?->c()?->null()?->d();
}

var_dump(null_chain_1(new A));
var_dump(null_chain_1(null));
var_dump(null_chain_2(new A));
var_dump(null_chain_2(null));
var_dump(null_chain_3(new A));
var_dump(null_chain_3(null));

function or_null(?A $a) {
  return $a?->self()?->bb?->self()?->c();
}
$res = or_null(getA());
if ($res) {
  var_dump($res->d());
}

A::$b_static = new B;
var_dump(A::$b_static?->null()?->c()?->self()?->d());
var_dump(A::get_b_static()?->null()?->c()?->self()?->d());