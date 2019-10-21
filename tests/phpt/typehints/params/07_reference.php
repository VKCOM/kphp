<?php
$ar = [1,2,3,4];
function test(array& $ar) {
  $ar[] = 5;
}

print_r($ar);

