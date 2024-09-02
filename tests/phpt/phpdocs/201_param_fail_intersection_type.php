@kphp_should_fail
\Intersection types are not supported\
<?php
class A {}
class B {}

/** @param A&B $obj */
function foo($obj) {
    // ...
}

$obj = new A(); // At this stage, the type for $obj isn't important
foo($obj);
