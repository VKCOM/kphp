@ok
<?php
function test(?int ...$args) {
  var_dump($args);
}

test(1,2,3,4,5,null, 7, null);


