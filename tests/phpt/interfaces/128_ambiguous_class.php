@kphp_should_fail
/Can't deduce class type, possible options are: C\$IB, C\$IA/
<?php

interface IA { function foo(); }
interface IB {}

class A implements IA, IB { function foo() {} }
class B implements IA, IB { function foo() {} }

$ab = new A();
$ab = new B();

