@kphp_should_fail
/invalid perl operator: \(\?\!/
<?php

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--confdata-blacklist" => "(?!xx)yy"
  ];
}
