@kphp_should_fail
/Expected '}' after x/
/Variable name expected/
<?php

// Tests with incomplete ${...} expression in strings.

var_dump("${x"); // missing closing '}'
var_dump("${"); // missing var name
