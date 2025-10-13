@ok
<?php
var_dump(hexdec("See"));
var_dump(hexdec("ee"));
// в обоих случаех будет выведено "int(238)"

var_dump(hexdec("that")); // выведет "int(10)"
var_dump(hexdec("a0")); // выведет "int(160)"
