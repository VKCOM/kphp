@ok
<?php

class BaseException extends \Exception {}
class DerivedException1 extends BaseException {}
class DerivedException2 extends BaseException {}

function test1(Exception $e) {
  try {
    throw $e;
  } catch (BaseException $e1) {
    var_dump([__LINE__ => $e1->getLine()]);
  } catch (DerivedException1 $e2) {
    var_dump([__LINE__ => $e2->getLine()]);
  } catch (DerivedException2 $e3) {
    var_dump([__LINE__ => $e3->getLine()]);
  } catch (Exception $e4) {
    var_dump([__LINE__ => $e4->getLine()]);
  }
  var_dump(__LINE__);
}

function test2(Exception $e) {
  try {
    throw $e;
  } catch (DerivedException1 $e2) {
    var_dump([__LINE__ => $e2->getLine()]);
  } catch (DerivedException2 $e3) {
    var_dump([__LINE__ => $e3->getLine()]);
  } catch (BaseException $e1) {
    var_dump([__LINE__ => $e1->getLine()]);
  } catch (Exception $e4) {
    var_dump([__LINE__ => $e4->getLine()]);
  }
  var_dump(__LINE__);
}

function test3(Exception $e) {
  try {
    throw $e;
  } catch (Exception $e4) {
    var_dump([__LINE__ => $e4->getLine()]);
  } catch (DerivedException1 $e2) {
    var_dump([__LINE__ => $e2->getLine()]);
  } catch (DerivedException2 $e3) {
    var_dump([__LINE__ => $e3->getLine()]);
  } catch (BaseException $e1) {
    var_dump([__LINE__ => $e1->getLine()]);
  }
  var_dump(__LINE__);
}

function test4(Exception $e) {
  try {
    throw $e;
  } catch (BaseException $e1) {
      var_dump([__LINE__ => $e1->getLine()]);
  } catch (Exception $e4) {
    var_dump([__LINE__ => $e4->getLine()]);
  } catch (DerivedException2 $e2) {
    var_dump([__LINE__ => $e2->getLine()]);
  } catch (DerivedException1 $e3) {
    var_dump([__LINE__ => $e3->getLine()]);
  }
  var_dump(__LINE__);
}

/** @var Exception[] */
$exceptions = [
  new DerivedException1(),
  new DerivedException2(),
  new BaseException(),
  new Exception(),
];

foreach ($exceptions as $e) {
  test1($e);
  test2($e);
  test3($e);
  test4($e);
}
