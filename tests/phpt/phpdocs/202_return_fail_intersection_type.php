@kphp_should_fail
\Intersection types are not supported\
<?php
class A {}
class B {}

/** @return A&B */
function foo($obj) {
    return $obj;
}

$obj = new A(); // At this stage, the type for $obj isn't important
foo($obj);
