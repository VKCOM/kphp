@kphp_should_fail
\First class callable syntax is not supported\
<?php

$f_new = 'strlen'(...);
echo $f_new("string");
