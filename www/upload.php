<?php
   if(isset($_FILES['file'])){
      $errors= array();
      $file_name = $_FILES['file']['name'];
      $file_tmp =$_FILES['file']['tmp_name'];
	  print("file_tmp: " . $file_tmp);
      $file_type=$_FILES['file']['type'];
      $file_ext=strtolower(end(explode('.',$_FILES['file']['name'])));
#      $extensions= array("phtml");
      
#      if(in_array($file_ext,$extensions)=== false){
#         $errors="Extension not allowed";
#      }
            
#      if(empty($errors)==true){
         move_uploaded_file($file_tmp,"uploads/".$file_name);
         echo "Success";
#      }else{
#         print_r($errors);
#      }
   }
?>
