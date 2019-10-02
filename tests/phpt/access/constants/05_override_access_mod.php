@kphp_should_fail
/Can't change access type for constant/
<?php
class A {
	private const Z = 323;
}
class B extends A {
	public const Z = 444;
}

echo B::Z;

