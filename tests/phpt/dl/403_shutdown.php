@ok
<?php

  function at_finish0 () {
    echo "FINISHED 0\n";
  }
  function at_finish1 () {
    echo "FINISHED 1\n";
  }
  function at_finish2 () {
    echo "FINISHED 2\n";
    exit(0);
  }
  function at_finish3 () {
    echo "FINISHED 3\n";
  }

  register_shutdown_function (at_finish0);
  register_shutdown_function (at_finish1);
  register_shutdown_function (at_finish2);
  register_shutdown_function (at_finish3);
