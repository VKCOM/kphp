@unsupported
<?php
	class FirstException extends Exception {}
	class SecondException extends Exception {}
	class ThirdException extends Exception {}
	
	function f()
	{
		static $x = 0;
		
		$x++;
		
		switch($x)
		{
			case 1: throw new FirstException();
			case 2: throw new SecondException();
			case 3: throw new ThirdException();
		}
	}

	try
	{
		f();
	}
	catch(FirstException $e)
	{
		echo "FirstException\n";
	}

	try
	{
		f();
	}
	catch(FirstException $e)
	{
		echo "FirstException\n";
	}
	catch(SecondException $e)
	{
		echo "SecondException\n";
	}
	
	try
	{
		f();
	}
	catch(FirstException $e)
	{
		echo "FirstException\n";
	}
	catch(SecondException $e)
	{
		echo "SecondException\n";
	}
	catch(ThirdException $e)
	{
		echo "ThirdException\n";
	}
?>
