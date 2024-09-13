@ok k2_skip
<?php

interface ITest {
  /**
   * @param string[] $args
   */
  public function run(...$args);
}

class TestClass implements ITest {
  /**
   * @param string[] $args
   */
  public function run(...$args) {
    var_dump($args);
  }
}

class Runner {
  /**
   * @param ITest $test
   */
  public function go(ITest $test) {
    $test->run("asdf","asdf","af");
  }
}

$testClass = new TestClass();
$runner = new Runner();
$runner->go($testClass);
