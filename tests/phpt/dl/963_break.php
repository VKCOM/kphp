@ok
<?

$arr_first = array (1 => "ab", 2 => "cd");
for (;;) {
  print "A\n";
  for (;;) {
    print "B\n";
    break 1;
    print "B'\n";
  }
  print "C\n";
  for (;;) {
    print "D\n";
    break 2;
    print "D'\n";
  }
  print "E\n";
}

print "-------\n";
$a = 1;
switch ($a) {
 case 1:
   print ("A\n");
   for (;;) {
     print ("B\n");
     break 2;
     print ("B'\n");
   }
   print ("C\n");
}
print "-------\n";
for ($i = 0; $i < 3; $i++) {
  print ("A\n");
  for ($j = 0; $j < 3; $j++) {
    print ("B\n");
    continue 2;
    print ("B'\n");
  }
  print ("A'\n");
}

print "-------\n";
foreach ($arr_first as $a) {
  print ("A\n");
  break 1;
  print ("B\n");
} 


foreach ($arr_first as $a) {
  switch ($a) {
  case 1:
  case 'ab':
    var_dump ($a);
    continue 2;
  default:
    var_dump ($a);
    continue 2;
  }
}


foreach(array(1, 2, 3) as $a) {
  switch($a) {
  case 1:
    continue;
  }
  var_dump ("$a");
}

function test() {
  foreach(array(1, 2, 3) as $a) {
    switch($a) {
    case 1:
      continue;
    }
  }

  
  do {
    foreach (array (1, 2, 3) as $a) {
      if ($a > 1) {
        break 2;
      }
    }
  } while (true);

}
test();
