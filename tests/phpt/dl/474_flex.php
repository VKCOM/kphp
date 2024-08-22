@ok k2_skip benchmark
<?php

$langs = array (0, 1, 2, 8, 11, 19, 21, 51, 52, 91, 100, 777, 888, 999);

for ($j = 0; $j < 1000; $j++)
foreach ($langs as $i) {
  var_dump($i.vk_flex("", 'Dat', 0, 'names', $i));
  var_dump($i.vk_flex("John", 'Dat', 0, 'names', $i));
  var_dump($i.vk_flex("asdasd-", 'Dat', 0, 'names', $i));
  var_dump($i.vk_flex("Алексей", 'Dat', 0, 'names', $i));
}
