@ok k2_skip
<?php

// https://github.com/VKCOM/kphp/issues/145
var_dump(preg_match('/\d+/', 'hello world', $m, 0, 100));
var_dump($m);
var_dump(preg_last_error());
