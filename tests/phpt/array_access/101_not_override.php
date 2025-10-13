@kphp_should_fail
/You should override abstract method ArrayAccess::offsetGet in class Sample/
/You should override abstract method ArrayAccess::offsetUnset in class Sample/
/You should override abstract method ArrayAccess::offsetSet in class Sample/
/You should override abstract method ArrayAccess::offsetExists in class Sample/
<?php 
class Sample implements \ArrayAccess {

}

function test() {
    $obj = new Sample();
    $obj[0] = '123';
}

test();
