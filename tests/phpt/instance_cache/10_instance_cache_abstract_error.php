@kphp_should_fail k2_skip
/Can not store polymorphic type AbstractClass with mutable derived class ConcreteB/
<?php

require_once 'kphp_tester_include.php';

/** @kphp-immutable-class
 *  @kphp-serializable */
abstract class AbstractClass {
    abstract protected function getValue();

    public function printOut() {
        print $this->getValue() . "\n";
    }
}

/** @kphp-immutable-class */
class ConcreteA extends AbstractClass {
    protected function getValue() {
        return "ConcreteA";
    }
}

class ConcreteB extends AbstractClass {
    protected function getValue() {
        return "ConcreteB";
    }
}

/** @kphp-immutable-class */
class ConcreteC extends ConcreteA {
    protected function getValue() {
        return "ConcreteC";
    }
}

function save_abstract(string $key, AbstractClass $value) {
    instance_cache_store($key, $value);
}


function test_abstract_class() {
    save_abstract("ConcreteA", new ConcreteA());
    save_abstract("ConcreteC", new ConcreteC());

    $x1 = instance_cache_fetch(AbstractClass::class, "ConcreteA");
    $x2 = instance_cache_fetch(AbstractClass::class, "ConcreteC");
}

test_abstract_class();
