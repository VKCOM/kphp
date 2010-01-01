@ok
<?php
  if(isset($a[1])) echo "\$a[1] is set\n"; else echo "\$a[1] is not set\n"; 
  $a[1] = 2;
  if(isset($a[1])) echo "\$a[1] is set\n"; else echo "\$a[1] is not set\n"; 
  $a[1] = NULL;
  if(isset($a[1])) echo "\$a[1] is set\n"; else echo "\$a[1] is not set\n"; 
?>
