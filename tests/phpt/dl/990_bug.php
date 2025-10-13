@ok
<?php
#had sigsev on this test
function f($d)
{
  if (!$d) return 1;
}
?>
