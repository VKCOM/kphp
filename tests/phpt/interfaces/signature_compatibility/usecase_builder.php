@kphp_should_fail
KPHP_REQUIRE_FUNCTIONS_TYPING=1
/Declaration of ExampleBuilder::create\(\) must be compatible with Builder::create\(\)/
<?php

// Builder::create() is not type-annotated, so it's implied
// that it returns void; ExampleBuilder::create overrides that
// type with ExampleBuilder and this it's incompatible with void

abstract class Builder {
  abstract public static function create();
}

class ExampleBuilder extends Builder {
  /** @return ExampleBuilder */
  public static function create() {
    return new ExampleBuilder();
  }
}

$b = ExampleBuilder::create();
