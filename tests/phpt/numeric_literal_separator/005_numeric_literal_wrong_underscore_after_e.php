@kphp_should_fail
/Bad numeric constant, '_' cannot go after 'e' or 'E'/
<?php

echo 10e_10;
