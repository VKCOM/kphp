@kphp_should_fail
/DEFAULT_RUNTIME_OPTIONS map values must be constexpr strings/
<?php

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--confdata-blacklist" => 123
  ];
}
