@kphp_should_fail
\Standalone true type is not supported\
<?php
/**@param true $a*/
 function foo($a) {
     return $a;
}
var_dump(foo(true));
