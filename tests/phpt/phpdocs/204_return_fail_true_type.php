@kphp_should_fail
\Standalone true type is not supported\
<?php
/**@return true $a*/
function test1(true $a) {
     return $a;
}
var_dump(foo(true));
