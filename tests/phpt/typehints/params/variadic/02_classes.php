@ok k2_skip
<?php
class ctest {
  public function empty(){}
}

function test(ctest ...$items) {
  foreach ($items as $item) {
    $item->empty();
  }
}

test(new ctest, new ctest, new ctest);
