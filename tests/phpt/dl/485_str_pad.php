@ok 
<?php
$input = "Alien";
echo str_pad($input, 10);                      // ������� "Alien     "
echo str_pad($input, 10, "-=", STR_PAD_LEFT);  // ������� "-=-=-Alien"
echo str_pad($input, 10, "_", STR_PAD_BOTH);   // ������� "__Alien___"
echo str_pad($input, 6 , "___");               // ������� "Alien_"
