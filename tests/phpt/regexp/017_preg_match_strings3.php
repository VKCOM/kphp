@ok
<?php

class Example {
  public static function test(string $types, string $s) {
    $matches = [];
    if (!preg_match('/(\w+)(' . $types . ')$/i', $s, $matches)) {
      var_dump(["no matches" => $matches]);
      return;
    }
    var_dump($matches);
  }
}

Example::test('', '');
Example::test('x', '32x');
Example::test('32x', 'x');
Example::test('a|b', '');
Example::test('a|b', '10a');
Example::test('a|b', '10b');
Example::test('a|b', '10');
Example::test('a|b', 'a');
Example::test('a|b', 'b');
