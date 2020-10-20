@kphp_should_fail
/DEFAULT_RUNTIME_OPTIONS\[--confdata-blacklist\] must be a constexpr string/
<?php

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--confdata-blacklist" => 123
  ];
}
