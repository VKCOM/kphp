@ok
<?php

class KphpConfiguration {
  const DEFAULT_RUNTIME_OPTIONS = [
    "--job-workers-shared-memory-distribution-weights" => "2, 1, 1, 1, 1, 1, 1, 1, 1, 1",
  ];
}

echo "successfully compiled\n";
