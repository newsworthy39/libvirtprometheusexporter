<?php
     $conn = libvirt_connect('null', false);
     $doms = libvirt_list_domains($conn);
     print_r($doms);


     libvirt_image_create($conn, 'test.img',8192, "qcow2");
?>
