@kphp_should_fail
/\$ab is assumed to be both A and B/
<?php

interface IA { function foo(); }
interface IB {}

class A implements IA, IB { function foo() {} }
class B implements IA, IB { function foo() {} }

$ab = new A();
$ab = new B();
$ab->foo();

