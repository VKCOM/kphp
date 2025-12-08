@ok benchmark
<?php
 var_dump (php_uname());
 var_dump (php_uname('s'));
 var_dump (php_uname('n'));
 var_dump (php_uname('r'));
 var_dump (php_uname('v'));
 var_dump (php_uname('m'));

  $images = array (
    "data/images/example.png",
    "data/images/example.gif",
    "data/images/example.jpg",
    "data/images/example-2.jpg",
    "data/images/example-3.jpg",
    "data/images/example-4.jpg",
    "data/images/nonexistent-image",
  );

  foreach ($images as $image) {
    var_dump ($image);
    var_dump (getimagesize ($image));
  }

