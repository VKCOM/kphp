@ok
<?php

for($x = 10; $x > 0; $x = $x - 1)
{
	$y = 2;
	$y = $y * $y;
}

for($x = 0, $y = 0; $x > 10; $x = $x + 1)
{
	$x = 3;
}

function f() {
  for(;;)
  {

  }

  for(;;) $x = $x + 1;

  for(;;);
}

//for($x = 0; $x < 10; $x = $x + 1):
//	$x = $y;
//endfor;

?>
