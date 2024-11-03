@ok k2_skip
<?php
#ifndef KPHP
?>
int(1)
int(2)
int(1)
<?php
exit;
#endif

$x = kphp_tracing_init("");
$level = kphp_tracing_get_level();
var_dump($level);
kphp_tracing_set_level(2);
var_dump(kphp_tracing_get_level());
kphp_tracing_set_level($level);
var_dump(kphp_tracing_get_level());
