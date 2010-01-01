@unsupported
<?php
	interface Foo 
	{
		const x = 5;
		public function f();
	}

	interface Bar
	{
		public function b();
	}

	interface FooBar extends Foo, Bar
	{
		public function fb();
	}

	interface Boo 
	{
		public function o();
	}

	abstract class D implements FooBar, Boo
	{
	}

	class C extends D 
	{
		public function f() { echo "f\n"; }
		public function b() { echo "b\n"; }
		public function fb() { echo "fb\n"; }
		public function o() { echo "o\n"; }
	}

	$c = new C();
	$c->f();
	$c->b();
	$c->fb();
	$c->o();
	echo Foo::x . "\n"; 
	echo FooBar::x . "\n"; 
	echo C::x . "\n"; 
?>
