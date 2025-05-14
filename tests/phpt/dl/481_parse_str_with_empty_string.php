@ok
<?php

$params = [];
parse_str("", $params);
var_dump($params);

$params = [];
parse_str(null, $params);
var_dump($params);

$params = [1,2,3];
parse_str('', $params);
var_dump($params);

$params = null;
parse_str(' ', $params);
var_dump($params);

$params = null;
parse_str('\t\n', $params);
var_dump($params);
