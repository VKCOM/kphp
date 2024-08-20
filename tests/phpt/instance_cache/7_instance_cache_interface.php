@ok non-idempotent k2_skip
<?php

require_once 'kphp_tester_include.php';

interface SimpleInterface {
    public function foo();
}

interface ComplexInterface extends SimpleInterface {
    public function bar();
}

/** @kphp-immutable-class */
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
class CompletelyComplex extends Complex {
  public function foo() {
    fwrite(STDERR, "foo\n");
  }
  public function bar() {
    fwrite(STDERR, "bar\n");
  }
}


/**
* @param $key string, @param $base Interface
*/
function save_interface(string $key, SimpleInterface $value) {
    instance_cache_store($key, $value);
}

function test_interface() {
    save_interface("simple", new Simple());
    save_interface("complex", new Complex());
    save_interface("completely_complex", new CompletelyComplex());

    $x1 = instance_cache_fetch(SimpleInterface::class, "simple");
    $x2 = instance_cache_fetch(SimpleInterface::class, "complex");
    $x3 = instance_cache_fetch(SimpleInterface::class, "completely_complex");

    if ($x1 instanceof Simple) {
        echo "successfully fetch Simple\n";
    }
    if ($x2 instanceof Complex) {
        echo "successfully fetch Complex\n";
    }
    if ($x3 instanceof CompletelyComplex) {
        echo "successfully fetch CompletelyComplex\n";
    }
}

test_interface();
