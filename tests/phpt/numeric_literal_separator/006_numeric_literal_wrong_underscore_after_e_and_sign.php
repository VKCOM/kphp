@kphp_should_fail
/Bad numeric constant, '_' cannot go after 'e<sign>' or 'E<sign>'/
<?php

echo 10e-_10;
