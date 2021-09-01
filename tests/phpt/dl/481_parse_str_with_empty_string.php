@ok
<?php

$params = [];
parse_str("", $params);
var_dump($params);

$params = [];
parse_str(null, $params);
var_dump($params);
