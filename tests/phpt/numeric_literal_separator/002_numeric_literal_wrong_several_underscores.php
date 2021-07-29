@kphp_should_fail
/Bad numeric constant, several '_' in a row are prohibited/
<?php

echo 100__100;
