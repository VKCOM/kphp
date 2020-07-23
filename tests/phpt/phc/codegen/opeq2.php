@ok
<?php
  // Nested opeq

  /**
   * @kphp-infer
   * @return int
   */
  function f()
  {
    echo "f is called\n";
    return 1;
  }

  $y[1] = 5;
  $x = ($y[f()] += 1);
  var_dump($x);
  var_dump($y);
?>
