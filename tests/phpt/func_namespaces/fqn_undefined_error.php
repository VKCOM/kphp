@kphp_should_fail
/Unknown function \\g/
/Unknown function \\f/
/Unknown function \\A\\B\\f()/
<?php

var_dump(\g());
var_dump(\f());
var_dump(\A\B\f());
