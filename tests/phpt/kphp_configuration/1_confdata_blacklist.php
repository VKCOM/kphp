@ok
<?php

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--confdata-blacklist" => "^lang(:?[^03]|\d\d+)_(:?4|42\d+|0\d+|[^04]\d*|4[^2]\d*)\..+$"
  ];
}

var_dump(KphpConfiguration::DEFAULT_RUNTIME_OPTIONS);
