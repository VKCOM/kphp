@ok
<?php
unset ($a);
unset ($b);
unset ($c);
echo
  "SELECT $a " .
  "FROM $b " .
  "GROUPBY $c";
?>
