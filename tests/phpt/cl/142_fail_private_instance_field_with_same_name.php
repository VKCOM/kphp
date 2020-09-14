@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

(new Classes\C_copy())->test_access_private_field_with_same_name();
