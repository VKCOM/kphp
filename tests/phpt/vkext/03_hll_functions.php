@ok
<?php

$hll_a = vk_stats_hll_create([1, 2, 3]);
$hll_b = vk_stats_hll_create([3, 4, 5]);

$hll = vk_stats_hll_merge([$hll_a, $hll_b]);
var_dump($hll);

$count = vk_stats_hll_count($hll);
var_dump($count);
