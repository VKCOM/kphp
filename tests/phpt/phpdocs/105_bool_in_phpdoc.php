@kphp_should_fail
/Do not use \|bool in phpdoc, use \|false instead\n\(if you really need bool, specify \|boolean\)/
<?php

/** @param string|bool $a */
function foo($a) {
  var_dump($a);
}

foo(false);
