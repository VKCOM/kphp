@kphp_should_fail
/acceptIntArr\(\[4, 'str'\]\);/
/array is mixed\[\]/
/"str" is string/
<?php

function acceptIntArr(int[] $arr) {}

acceptIntArr([4, 'str']);
