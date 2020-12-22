@ok
<?php

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--warmup-workers-ratio" => "0.1",
    "--warmup-instance-cache-elements-ratio" => "0.5",
    "--warmup-timeout" => "10.0",
  ];
}

echo "successfully compiled\n";
