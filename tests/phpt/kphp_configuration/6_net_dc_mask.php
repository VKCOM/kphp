@ok
<?php

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--net-dc-mask" => [
      "9=127.0.0.0/8",
      "3=192.168.0.0/16",
    ]
  ];
}

echo "successfully compiled\n";
// no runtime test for parsed and applied correctly though
