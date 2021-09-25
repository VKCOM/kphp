@kphp_should_fail
/Unknown method Classes\\NamespaceTester::f123/
/Unknown method A::demo/
/Unknown class VK\\SomeNs\\A, can't call its method/
/Unknown function asdfasdf/
<?php

class A {
}

Classes\NamespaceTester::f123();
A::demo();
VK\SomeNs\A::demo();
asdfasdf();
