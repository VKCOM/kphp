@ok benchmark
<?php

require_once dirname(__FILE__) . '/include/test-json.php';

test_json (JSON_UNESCAPED_UNICODE);
test_json (JSON_UNESCAPED_SLASHES);
