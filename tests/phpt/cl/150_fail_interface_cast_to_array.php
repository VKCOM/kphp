@kphp_should_fail
/interface to array/
<?php

#ifndef KittenPHP
function instance_to_array($instance) { return (array) $instance; }
#endif

interface IFoo { public function foo(); }
class Foo implements IFoo { public function foo() {} }
class Bar implements IFoo { public function foo() {} }
/** @var IFoo */
$foo = new Foo();
$foo = new Bar();

$arr_foo = instance_to_array($foo);
