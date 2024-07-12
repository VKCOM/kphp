@ok k2_skip
<?php

  function xxx () {
    print __FUNCTION__."\n";
    print __LINE__."\n";
  }
  print __LINE__."\n";
  print __FILE__."\n";
  print __LINE__."\n";
  print __FILE__."\n";
  xxx();
  print __FUNCTION__."\n";
  print __FILE__."\n";
?>
