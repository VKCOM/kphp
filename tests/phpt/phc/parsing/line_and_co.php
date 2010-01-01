@todo
<?php
	$x = __LINE__;
//	echo "$x\n";
	$x = __FILE__;
	echo "$x\n";
	$x = __FUNCTION__;
	echo "$x\n";
	$x = __METHOD__;
	echo "$x\n";
	$x = __CLASS__;
	echo "$x\n";

	function some_function()
	{
		$x = __LINE__;
//		echo "$x\n";
		$x = __FILE__;
		echo "$x\n";
		$x = __FUNCTION__;
		echo "$x\n";
		$x = __METHOD__;
		echo "$x\n";
		$x = __CLASS__;
		echo "$x\n";

		class A
		{
			function some_method ()
			{
				$x = __LINE__;
//				echo "$x\n";
				$x = __FILE__;
				echo "$x\n";
				$x = __FUNCTION__;
				echo "$x\n";
				$x = __METHOD__;
				echo "$x\n";
				$x = __CLASS__;
				echo "$x\n";
			}
		}
	}

	class SOME_CLASS
	{
		public function some_method() 
		{
			$x = __LINE__;
//			echo "$x\n";
			$x = __FILE__;
			echo "$x\n";
			$x = __FUNCTION__;
			echo "$x\n";
			$x = __METHOD__;
			echo "$x\n";
			$x = __CLASS__;
			echo "$x\n";
		}
	}
	
	$x = __LINE__;
//	echo "$x\n";
	$x = __FILE__;
	echo "$x\n";
	$x = __FUNCTION__;
	echo "$x\n";
	$x = __METHOD__;
	echo "$x\n";
	$x = __CLASS__;
	echo "$x\n";


	some_function ();
	$y = new SOME_CLASS ();
	$y->some_method ();
	$z = new A ();
	$z->some_method ();
?>
