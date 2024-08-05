@kphp_should_fail k2_skip
/Can't catch Foo; only classes that implement Throwable can be caught/
<?php

class Foo {}

try {
} catch (Foo $foo) {
}
