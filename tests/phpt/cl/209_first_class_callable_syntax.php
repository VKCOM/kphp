@kphp_should_fail
\First class callable syntax is not supported\
<?php

$f_new = Classes\G::static_private_func(...);
echo $f_new();
