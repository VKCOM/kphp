@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

$a = ' €‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—™š›œžŸ!¡"¢#Ž$¤%¥&¦\'§(¨)©*ª+«,¬-­.®/¯0°1±2²3³4´5µ6¶7·8¸9¹:º;»<¼=½>¾?¿@ÀAÁBÂCÃDÄEÅFÆGÇHÈIÉJÊKËLÌMÍNÎOÏPÐQÑRÒSÓTÔUÕVÖW×XØYÙZÚ[Û\Ü]Ý^Þ_ß`àaábâcãdäeåfægçhèiéjêkëlìmínîoïpðqñròsótôuõvöw÷xøyùzú{û|ü}ý~þÿ';
$serialized = msgpack_serialize($a);
var_dump(base64_encode($serialized));

$a_new = msgpack_deserialize($serialized);
var_dump($a === $a_new);
var_dump($a_new);

$a = 'here i am';
$serialized = msgpack_serialize($a);
var_dump(base64_encode($serialized));
$a_new = msgpack_deserialize($serialized);
var_dump($a === $a_new);
var_dump($a_new);
