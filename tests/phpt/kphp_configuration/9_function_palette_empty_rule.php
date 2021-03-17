@kphp_should_fail
/KphpConfiguration\:\:FUNCTION_PALETTE rule must not be empty/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    "" => 1,             // error, empty rule
  ];
}
