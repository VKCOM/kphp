@kphp_should_fail
/pass null to argument \$x of f/
/declared as @param Foo/
<?php

class Foo {}

function f(Foo $x) {}

f(null);
