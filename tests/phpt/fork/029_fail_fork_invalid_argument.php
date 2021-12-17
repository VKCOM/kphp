@kphp_should_fail
/Invalid fork\(\) usage: it must be called with exactly one func call inside/
<?php

fork(1);
