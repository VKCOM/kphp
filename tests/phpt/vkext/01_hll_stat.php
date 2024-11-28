@ok
<?php
function eq_in_range($expected, $actual, $delta)
{
  return (($expected + $delta) >= $actual) && (($expected - $delta) <= $actual);
}

var_dump(false === vk_stats_hll_create([], 1));
var_dump('0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000' === vk_stats_hll_create([1], 256));
var_dump('0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000' === vk_stats_hll_create([1, 1, 1], 256));
var_dump('0000100000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000' === vk_stats_hll_create(range(1, 3), 256));
var_dump('0001230122301002380002031052112121204202311222110301201301010022205423100221204104410730005401011210130220011000001040101426000240121070030000220200002011230022010000230300110031212043601110020014=11261030114000201328111<00320333172203041043154030301021331' === vk_stats_hll_create(range(1, 253), 256));

var_dump(false === vk_stats_hll_count('1'));

var_dump(eq_in_range(1, vk_stats_hll_count(vk_stats_hll_create([1])), 1E-1));
var_dump(eq_in_range(1, vk_stats_hll_count(vk_stats_hll_create([1, 1])), 1E-1));
var_dump(eq_in_range(2, vk_stats_hll_count(vk_stats_hll_create([1, 2])), 1E-1));
var_dump(eq_in_range(253.77, vk_stats_hll_count(vk_stats_hll_create(range(1, 253))), 1E-1));

$hll1 = vk_stats_hll_create([1, 2]);
$hll2 = vk_stats_hll_create([1, 2, 3]);
var_dump(eq_in_range(3, vk_stats_hll_count(vk_stats_hll_merge([$hll1, $hll2])), 1E-1));

$hll3 = vk_stats_hll_create([4, 5, 6]);
var_dump(eq_in_range(6, vk_stats_hll_count(vk_stats_hll_merge([$hll2, $hll3])), 1E-1));

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
