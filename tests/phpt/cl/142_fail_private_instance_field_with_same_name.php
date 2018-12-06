@kphp_should_fail
<?php
require_once 'Classes/autoload.php';

(new Classes\C_copy())->test_access_private_field_with_same_name();
