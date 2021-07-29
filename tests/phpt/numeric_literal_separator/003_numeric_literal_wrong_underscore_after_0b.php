@kphp_should_fail
/Bad numeric constant, '_' cannot go after '0b'/
<?php

echo 0b_100_100;
