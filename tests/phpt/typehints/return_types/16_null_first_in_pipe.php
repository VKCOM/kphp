@ok
<?php

class BaseClass {
  public function call(): void {}
}

class ChildClass1 extends BaseClass {}
class ChildClass2 extends BaseClass {}

/** @return null|ChildClass1|ChildClass2 */
function execute() {
  return new ChildClass1();
}

/** @return null|ChildClass1|null|ChildClass2 */
function execute2() {
    return new ChildClass2();
}

/** @return null|null|null */
function execute3() {
    return null;
}

function main(): void {
    execute()->call();
    execute2()->call();
    var_dump(execute3());
}
  
main();
