@ok
<?php
require_once 'kphp_tester_include.php';


interface IData {
	function get(): string;
}
class DataA implements IData {
	function get(): string { return "DataA"; }
}
class DataB implements IData {
	function get(): string { return "DataB"; }
}

interface I
{
	/**
	* @param IData[] $a
	* @return IData[]
	*/
	public function method(array $a);
}

class A implements I
{
	public function method(array $a) {
		return [new DataA()];
	}
}

class B extends A
{
	public function method(array $a) {
		return $this->getB();
	}

	/** @return DataB[] */
	function getB(): array  {
		return [new DataB()];
	}
}

function test2(I $i) {
    foreach ($i->method([new DataB()]) as $idata) {
        echo $idata->get(), "\n";
    }
}

test2(new A);
test2(new B);


interface IWorkWithData {
    /**
      * @param IData[] $idata
      * @return IData[]
      */
    function showArray(array $idata): array;

    /**
     * @param callable(IData):IData $cb
     */
    function callAndShow(callable $cb);
}

class Work implements IWorkWithData {
    function showArray(array $idata): array {
        foreach ($idata as $i)
            echo $i->get(), "\n";
        return [];
    }

    function callAndShow(callable $cb) {
        echo $cb(new DataA)->get(), "\n";
    }
}

function demo1() {
    $w = new Work;
    $w->showArray([new DataA, new DataB]);
    $w->callAndShow(function ($a) { $a->get(); return $a; });
}

demo1();

