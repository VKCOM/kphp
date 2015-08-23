@ok
<?php

ob_start();
echo ob_get_level() . " " . ob_get_length() . "\n";
ob_start();
echo ob_get_level() . " " . ob_get_length() . "\n";
ob_end_flush();
echo ob_get_level() . " " . ob_get_length() . "\n";
ob_flush();
echo ob_get_level() . " " . ob_get_length() . "\n";
$res = ob_get_flush();
echo "res = $res"; 
var_dump(ob_get_level(), ob_get_length());
