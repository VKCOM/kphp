@ok
<?php
	// Added to a function which we don't execute so that this script is
	// (trivially) executable. This is an unparser test, not a codegen test.
	function f()
	{
		echo "hi";
		echo("hi");

		echo "hi", "foo";
		#echo("hi", "foo"); // apparently this is invalid syntax!
		
		print "bar";
		print("bar");

		#include "foo";
		#include_once "foo";
		require "foo";
		require_once "foo";
		// use "foo";
		
		#include("foo");
		#include_once("foo");
		require("foo");
		require_once("foo");
		// use("foo");

		#$x = clone $y;
		#$x = clone($y);

		exit;
		exit();
		#exit(1);

		#$x = new C;
		#$x = new C();
		#$x = new C(0);
	}
?>
