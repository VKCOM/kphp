@pass
<?php
switch ($act) {
case "one":
  print "one";
  return 1;
case "two":
  print "two";
  break; 
case "three":
  print "three";
case "four":
  switch ($act) {
  default:
    var_dump ("oppa");
    continue;
  }
  print "four";
};

return -1;
