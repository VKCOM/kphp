@kphp_should_fail
/Redeclaration of class Classes\\NonAutoloadable, the previous declaration was in/
<?php
require_once 'kphp_tester_include.php';

var_dump(Classes\A_with_non_autloload::FOO);
var_dump(Classes\B_with_non_autloload::FOO);
