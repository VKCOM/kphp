@ok
<?php
  $s = "abcd";
  switch ($s) {
    case "abc":
      print 1;
    if (1) break;
    case "abcd":
      print 3;
      break;

    case "zasdf":
      print 2;
      break;
    
    default:
      print 4;
  }
?>
