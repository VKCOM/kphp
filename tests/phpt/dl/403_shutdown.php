@ok
<?php

  function at_finish () {
    echo "FINISHED\n";
  }

  register_shutdown_function (at_finish);
