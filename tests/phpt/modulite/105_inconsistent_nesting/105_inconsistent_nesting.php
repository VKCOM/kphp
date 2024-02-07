@kphp_should_fail
/Utils105\/.modulite.yaml:-1/
/inconsistent nesting: @msg\/utils placed outside of @msg/
/inconsistent nesting: @feed placed in @msg/
/inconsistent nesting: @msg\/channels\/infra placed in @msg/
<?php

require_once __DIR__ . '/Messages105/Messages105.php';
require_once __DIR__ . '/Messages105/Sub105/Channels105/Channels105.php';
require_once __DIR__ . '/Messages105/Infra105/Infra105.php';
require_once __DIR__ . '/Messages105/Feed105/Feed105.php';
require_once __DIR__ . '/Utils105/Strings105.php';

empty_call_will_fail_before();
