@kphp_should_fail php8
Expected expression before '=>'
<?php

match(10) {
   => "ok",
};
