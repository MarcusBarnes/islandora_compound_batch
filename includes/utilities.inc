<?php

/**
 * @file
 * Utility data and methods class
 */

/**
 * Utilies class 
 */
class Utilies {

    /**
     * Extension to Islandora content model map
     * From: https://gist.github.com/mjordan/f1c8e84beb312e88c239
     */
    public $extensionToContentModelMap = array();
    
    public function __construct(){
        $this->extensionToContentModelMap = array(
                                              'jpeg' => 'islandora:sp_basic_image',
                                              'jpe' => 'islandora:sp_basic_image', 
                                              'jpeg' => 'islandora:sp_basic_image', 
                                              'gif' => 'islandora:sp_basic_image', 
                                              'png' => 'islandora:sp_basic_image',
                                              'tif' => 'islandora:sp_large_image_cmodel',
                                              'tiff' => 'islandora:sp_large_image_cmodel',
                                              'jp2' => 'islandora:sp_large_image_cmodel',
                                              'pdf' => 'islandora:sp_pdf',
                                              'mp3' => 'islandora:sp-audioCModel',
                                              'mp4a' => 'islandora:sp-audioCModel',
                                              'm4a' => 'islandora:sp-audioCModel',
                                              'oga' => 'islandora:sp-audioCModel',
                                              'ogg' => 'islandora:sp-audioCModel',
                                              'flac' => 'islandora:sp-audioCModel',
                                              'wav' => 'islandora:sp-audioCModel',
                                              'mp4' => 'islandora:sp_videoCModel',
                                              'm4v'  => 'islandora:sp_videoCModel',
                                              'mkv'  => 'islandora:sp_videoCModel',
                                              'mpeg' => 'islandora:sp_videoCModel',
                                              'mpe' => 'islandora:sp_videoCModel',
                                              'mpg' => 'islandora:sp_videoCModel',
                                              'qt' => 'islandora:sp_videoCModel',
                                              'mov' => 'islandora:sp_videoCModel',
                                              'ogv' => 'islandora:sp_videoCModel',
                                            );
    }
    
    /**
     * Returns an array of extensions in the Extension to Islandora content model map
     */
    public function getObjExtensions(){
        return array_keys($this->extensionToContentModelMap);        
    }
    
    /**
     * Checks if the $ext file extension is in the Extension to Islandora content model map.
     * Returns TRUE if it is and FALSE otherwise.
     */         
    public function extInContentModelMap($ext) {
        return in_array($ext,$this->getObjExtensions());
    }


}