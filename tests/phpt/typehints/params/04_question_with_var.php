@kphp_should_fail
/Syntax error: question token without type specifier/
<?php
function foo(? $var) {
}

foo(null);
