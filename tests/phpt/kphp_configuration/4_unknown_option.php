@kphp_should_fail
/Got unexpected option KphpConfiguration/
<?php

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--unknown-option" => "x"
  ];
}
