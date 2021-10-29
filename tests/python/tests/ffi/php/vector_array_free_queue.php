<?php

class ClearQueue {
  /** @var (callable(): void)[] */
  private static $funcs = [];

  /** @param callable(): void $f */
  public static function push(callable $f) {
    self::$funcs []= $f;
  }

  public static function run() {
    foreach (self::$funcs as $f) {
      $f();
    }
  }
};

function test_vector_array() {
  $lib = FFI::scope('vector');

  $array = $lib->new('struct Vector2Array');
  $lib->vector2_array_alloc(FFI::addr($array), 10);
  ClearQueue::push(function() use ($lib, $array) {
    var_dump('free array 1');
    $lib->vector2_array_free(FFI::addr($array));
  });

  $array2 = $lib->new('struct Vector2Array');
  $lib->vector2_array_alloc(FFI::addr($array2), 20);
  ClearQueue::push(function() use ($lib, $array2) {
    var_dump('free array 2');
    $lib->vector2_array_free(FFI::addr($array2));
  });

  $array3 = $lib->new('struct Vector2Array');
  $lib->vector2_array_alloc(FFI::addr($array3), 10);
  ClearQueue::push(function() use ($lib, $array3) {
    var_dump('free array 3');
    $lib->vector2_array_free(FFI::addr($array3));
  });
}

#ifndef KPHP
if (false)
#endif
  FFI::load(__DIR__ . '/../c/vector.h');

register_shutdown_function(function() {
  ClearQueue::run();
});

test_vector_array();
