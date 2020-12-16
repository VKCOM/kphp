@ok
<?php

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--warmup-workers-part" => "0.1",
    "--warmup-instance-cache-elements-part" => "0.5",
    "--warmup-timeout-sec" => "10",
  ];
}

echo "successfully compiled\n";
