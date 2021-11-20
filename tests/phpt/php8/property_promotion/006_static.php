@kphp_should_fail php8
/Syntax error: missing varname after typehint/
<?php

class Test {
     public function __construct(public static string $x) {}
}
