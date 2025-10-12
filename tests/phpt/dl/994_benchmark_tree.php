@ok benchmark
<?php
function bottomUpTree($item, $depth)
{
   if (!$depth) return array(null,null,$item);
   $item2 = $item + $item;
   $depth--;
   return array(
      bottomUpTree($item2-1,$depth),
      bottomUpTree($item2,$depth),
      $item);
}
function itemCheck($treeNode) {
   return $treeNode[2]
      + ($treeNode[0][0] == null ? itemCheck($treeNode[0]) : $treeNode[0][2])
      - ($treeNode[1][0] == null ? itemCheck($treeNode[1]) : $treeNode[1][2]);
}

$minDepth = 4;

$n = 10;
if ($minDepth + 2 > $n) {
  $maxDepth = $minDepth + 2;
} else {
  $maxDepth = $n;
}
$stretchDepth = $maxDepth + 1;

$stretchTree = bottomUpTree(0, $stretchDepth);
$tmp = itemCheck($stretchTree);
echo ("stretch tree of depth {$stretchDepth}\t check: $tmp\n");
unset($stretchTree);

$longLivedTree = bottomUpTree(0, $maxDepth);

$one = 1;
$iterations = $one << ($maxDepth);
do
{
   $check = 0;
   for($i = 1; $i <= $iterations; ++$i)
   {
      $t = bottomUpTree($i, $minDepth);
      $check = $check + itemCheck($t);
      unset($t);
      $t = bottomUpTree(-$i, $minDepth);
      $check = $check + itemCheck($t);
      unset($t);
   }
   
   
   $tmp = $iterations<<1;
   echo("$tmp\t trees of depth $minDepth\t check: $check\n");
   
   $minDepth = $minDepth + 2;
   $iterations = $iterations >> 2;
}
while($minDepth <= $maxDepth);

$tmp = itemCheck($longLivedTree);
echo ("long lived tree of depth {$stretchDepth}\t check: $tmp\n");
?>
