<?php
	echo "Hello from CGI, " .getenv("USER"). "\n";
	$name = $_GET["name"];
	$pass = $_GET["password"];

	echo "Welcome $name, your password is $pass; don't share it with anyone";
	sleep(1);
?>
