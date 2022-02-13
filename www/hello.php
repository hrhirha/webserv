<?php
	echo "Hello from CGI, " .getenv("USER"). "\n";
	$name = $_POST["name"];
	$pass = $_POST["password"];

	echo "Welcome $name, your password is $pass; don't share it with anyone";
	sleep(61);
	var_export($_SERVER);
?>
