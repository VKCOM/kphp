@kphp_should_fail
/Can not store polymorphic type SimpleInterface with mutable derived class Simple/
<?php

require_once 'kphp_tester_include.php';

interface SimpleInterface {
    public function foo();
}

interface ComplexInterface extends SimpleInterface {
    public function bar();
}

class Simple implements SimpleInterface {
  public function foo() {
    fwrite(STDERR, "foo\n");
  }
}

/** @kphp-immutable-class */
class Complex implements ComplexInterface {
  public function foo() {
    fwrite(STDERR, "foo\n");
  }
  public function bar() {
    fwrite(STDERR, "bar\n");
  }
}

/** @kphp-immutable-class */
class CompletelyComplex implements ComplexInterface {
  public function foo() {
    fwrite(STDERR, "foo\n");
  }
  public function bar() {
    fwrite(STDERR, "bar\n");
  }
}

function save_interface(string $key, SimpleInterface $value) {
    instance_cache_store($key, $value);
}

function test_interface() {
    save_interface("CompletelyComplex", new CompletelyComplex());

    $x = instance_cache_fetch(CompletelyComplex::class, "CompletelyComplex");
}

test_interface();
