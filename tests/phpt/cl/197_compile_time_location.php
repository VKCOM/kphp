@ok
<?php

require_once 'kphp_tester_include.php';

function f(CompileTimeLocation $loc = null) {
    $loc = CompileTimeLocation::calculate($loc);
    $basename = basename($loc->file);
    echo "f() called from ", $basename, ' ', $loc->function, ':', $loc->line, "\n";
}

function log_info(string $message, CompileTimeLocation $loc = null) {
    $loc = CompileTimeLocation::calculate($loc);
    $basename = basename($loc->file);
    echo "$message (in {$loc->function} at {$basename}:{$loc->line})\n";
}

function my_log_more(string $message, ?CompileTimeLocation $loc = null) {
    $loc = CompileTimeLocation::calculate($loc);
    log_info("!!" . $message, $loc);
}

function demo1() {
    f();
    f();
}

class A {
    static function demo2() {
        f();
    }

    function demo3() {
        f();
    }
}

function demo4() {
    (function() {
        f();
    })();
}

function demo5() {
    log_info("start calculation");
    $arr = [1,2,3,4,5];
    foreach ($arr as $v) {
        if ($v > 3)
            log_info("big v=$v");
    }
    log_info("end calculation");
}

function demo6() {
    my_log_more("start calculation");
    $arr = [1,2,3,4,5];
    foreach ($arr as $v) {
        if ($v > 3)
            my_log_more("big v=$v");
    }
    my_log_more("end calculation");
}

function custom_log_1(int $log_level, string $message = "my_msg", ?CompileTimeLocation $loc = null) {
  $loc = CompileTimeLocation::calculate($loc);
  $basename = basename($loc->file);

  echo $log_level;
  echo "$message (in {$loc->function} at {$basename}:{$loc->line})\n";
}

function custom_bad_log(?CompileTimeLocation $loc = null, int $log_level = 1, string $message = "my_msg2") {
  echo $log_level;
  echo "$message \n";
  if ($loc !== null) {
    $basename = basename($loc->file);
    echo "$message (in {$loc->function} at {$basename}:{$loc->line})\n";
  }
}

function custom_good_log(int $log_level = 1, ?CompileTimeLocation $loc = null, string $message = "my_msg3") {
  echo $log_level;
  $loc = CompileTimeLocation::calculate($loc);
  $basename = basename($loc->file);
  echo "$message (in {$loc->function} at {$basename}:{$loc->line})\n";
}

function two_locations(string $message = "msg", CompileTimeLocation $loc1 = null, ?CompileTimeLocation $loc2 = null, int $more = 0) {
  $loc1 = CompileTimeLocation::calculate($loc1);
  $loc2 = CompileTimeLocation::calculate($loc2);
  echo $message, ' ', $loc1->function, ' ', $loc2->function, ' ', $loc1->line, ' ', $loc2->line, "\n";
}
function callTwoLocations() {
  two_locations();
  two_locations("ms");
}

demo1();
A::demo2();
(new A)->demo3();
demo4();
f();
demo5();
demo6();
my_log_more('end');

custom_log_1(1);
custom_log_1(2, "custom msg");

custom_bad_log(null);
custom_bad_log(null, 42);
custom_bad_log(null, 42, "custom msg2");

custom_good_log(2);

callTwoLocations();
