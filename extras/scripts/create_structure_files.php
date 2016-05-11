<?php
/**
 * Adds strucutre file to root of compound ojects.
 * path_to_directory_containing_compound_objects\
 *    compound_object_1\
 *    compound_object_2\
 *    compound_object_3\
 *    .
 *    .
 *    .
 *
 *
 * Usage:
 *
 * > php create_strcutre_files.php path_to_directory_containing_compound_objects 
 * 
 *
 *
 */ 


$target_directory = trim($argv[1]);

if(!is_dir($target_directory)){
    exit("Please check that you have provided a full path to a directory as the input argument." . PHP_EOL);
}

scanWrapperDirectory($target_directory);

function scanWrapperDirectory($target_directory, $structurefilename = 'structure') {
    //  basenames to exclude.
    $exclude_array = array('..', '.DS_Store', 'Thumbs.db', '.');

    $stuffinwrapperdirectory = scandir($target_directory);
    foreach ($stuffinwrapperdirectory as $compoundObjectOrFile) {
        $objpath = $target_directory . DIRECTORY_SEPARATOR . $compoundObjectOrFile;
        if(!in_array($compoundObjectOrFile, $exclude_array) && is_dir($objpath)) {
           // subdirectories of wrapper directory will be compound object.
           // create a structure file for each.
           $structure_xml = compoundObjectStructureXML($objpath);
           
           $structure_xml_output_file_path = $objpath . DIRECTORY_SEPARATOR 
                                            . $structurefilename . '.xml';
           file_put_contents($structure_xml_output_file_path, $structure_xml);
           
        }
        
    }
}


/** 
 * Recursively create XML string of directory structure/
 * Based on psuedo-code from http://stackoverflow.com/a/15096721/850828 
 */
function directoryXML($directory_path) {
    
    //  basenames to exclude.
    $exclude_array = array('..', '.DS_Store', 'Thumbs.db', '.');
    
    $dir_name = basename($directory_path);
    $xml = "<directory name='" . $dir_name . "'>";
    
    $pathbase = pathinfo($directory_path, PATHINFO_BASENAME);
    $stuffindirectory = scandir($directory_path);
    
    foreach($stuffindirectory as $subdirOrfile){
        
        $subdirOrfilepath = $directory_path . DIRECTORY_SEPARATOR  . $subdirOrfile;
        
        if(!in_array($subdirOrfile, $exclude_array) && is_file($subdirOrfilepath)){
          $xml .= "<file name='". $subdirOrfile . "' />";
        
        }
    
        if(!in_array($subdirOrfile, $exclude_array) && is_dir($subdirOrfilepath)){
            $xml .= directoryXML($subdirOrfilepath);        
        }
        
    }
    $xml .= "</directory>";
    return $xml;
}

function compoundObjectStructureXML($dir_path) {
    $xmlstring = "<tree>";
    $xmlstring .= directoryXML($dir_path);
    $xmlstring .= "</tree>";
    $xml = new DOMDocument( "1.0");
    $xml->loadXML($xmlstring);
    $xml->formatOutput = true;
    return $xml->saveXML();
}

?>