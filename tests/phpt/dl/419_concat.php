@ok benchmark
<?php
 for ($iteration = 0; $iteration < 1000; $iteration++) {
   $index = "1".($iteration ? ("_".$iteration) : "");
 }
 var_dump (strlen ($index));
