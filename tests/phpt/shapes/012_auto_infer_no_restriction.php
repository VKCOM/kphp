@ok
<?php
require_once 'kphp_tester_include.php';


// $sh if auto-inferred as shape(a:int|null, b:int|null)
/**
 * @param shape(a:int|null, b:int|null) $sh
 */
function f($sh) {
  var_dump($sh['a']);
  var_dump($sh['b']);
}

f(shape(['a' => 1]));
f(shape(['b' => 3]));


// same for f2, but more complex
/**
 * @param shape(a:mixed, b:mixed, c:string|null, d:string|null) $sh
 */
function f2($sh) {
  var_dump($sh['a']);
  var_dump($sh['b']);
  var_dump($sh['c']);
  var_dump($sh['d']);
}

f2(shape(['a' => 1]));
f2(shape(['a' => 's', 'b' => []]));
f2(shape(['c' => 's', 'b' => 1, 'a' => 's']));
f2(shape(['d' => 's', 'c' => 's']));


// same for return of f3 (a?:int, b?:int, c:string)
/**
 * @return shape(a:int|null, b:int|null, c:string)
 */
function f3() {
  if(1)
    return shape(['a' => 1, 'c' => 'c']);
  else if(1)
    return shape(['c' => 'c', 'b' => 1]);
  else
    return shape(['c' => 'c']);
}

f2(f3());
