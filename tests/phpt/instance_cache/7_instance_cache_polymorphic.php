@ok non-idempotent
<?php

require_once 'kphp_tester_include.php';

/** @kphp-immutable-class */
class SimpleBase {
    public $base_id = 1;
}

/** @kphp-immutable-class */
class SimpleDerived extends SimpleBase {
    public $derived_id = 2;
}

interface SimpleInterface {
  public function foo();
}

class InterfaceRealisation1 implements SimpleInterface {
  /** @var int */
  public $x = 1;
  public function foo() {
    var_dump($this->x);
  }

  public function __toString() {
    return __CLASS__;
  }
}

class InterfaceRealisation2 implements SimpleInterface {
  /** @var int */
  public $x = 2;
  public function foo() {
    var_dump($this->x);
  }

  public function __toString() {
    return __CLASS__;
  }
}

abstract class AbstractClass {
    abstract protected function getValue();

    public function printOut() {
        print $this->getValue() . "\n";
    }
}

class ConcreteClass1 extends AbstractClass {
    protected function getValue() {
        return "ConcreteClass1";
    }
}

class ConcreteClass2 extends AbstractClass {
    public function getValue() {
        return "ConcreteClass2";
    }
}


/**
* @param $key string, @param $base SimpleBase
*/
function save_derive_as_base($key, $base) {
    instance_cache_store($key, $base);
}

/**
* @param $key string, @param $base Interface
*/
function save_interface($key, $value) {
    instance_cache_store($key, $value);
}

/**
* @param $key string, @param $base AbstractClass
*/
function save_abstract($key, $value) {
    instance_cache_store($key, $value);
}

function test_base_polymorphic() {
    $derive = new SimpleDerived();
    instance_cache_store("simple_key", $derive);

    $res = instance_cache_fetch(SimpleDerived::class, "simple_key");
    var_dump($res->derived_id);
}

function test_save_derive_as_base() {
    $derive = new SimpleDerived();
    $base = new SimpleBase();
    save_derive_as_base("simple_derive_key", $derive);
    save_derive_as_base("simple_base_key", $base);

    $res = instance_cache_fetch(SimpleBase::class, "simple_derive_key");
    if ($res instanceof SimpleDerived) {
        var_dump($res->derived_id);
    }
}

function test_interface() {
    save_interface("interface_1", new InterfaceRealisation1());
    save_interface("interface_2", new InterfaceRealisation2());

    $x1 = instance_cache_fetch(IX::class, "interface_1");
    $x2 = instance_cache_fetch(IX::class, "interface_2");
    $x1->foo();
    $x2->foo();

    if ($x1 instanceof InterfaceRealisation1) {
        echo "$x1 is InterfaceRealisation1\n";
    }
    if ($x2 instanceof InterfaceRealisation2) {
        echo "$x2 is InterfaceRealisation2\n";
    }
}

function test_abstract_class() {

}

test_base_polymorphic();
test_save_derive_as_base();
test_interface();