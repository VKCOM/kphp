@ok benchmark
<?php

require_once dirname(__FILE__) . '/include/test-json.php';

test_json (0);
test_json (JSON_PARTIAL_OUTPUT_ON_ERROR);
