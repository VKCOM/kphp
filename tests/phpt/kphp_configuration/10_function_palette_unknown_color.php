@kphp_should_fail
/KphpConfiguration\:\:FUNCTION_PALETTE unknown 'some_color' color/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
     "some_color api" => 1, // unknown color
  ];
}
