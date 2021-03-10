@kphp_should_fail
/Declaration of B4::method\(\) must be compatible with A4::method\(\)/
<?php

// Zend/tests/type_declarations/variance/parent_in_class_failure2.phpt

// Illegal: B4::parent == A4 is subclass of A4::parent == P4 in contravariant position
class P4 {}
class A4 extends P4 {
    public function method(parent $x) {}
}
class B4 extends A4 {
    public function method(parent $x) {}
}
