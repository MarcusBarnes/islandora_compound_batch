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

$path_to_xsl =  "tree_to_compound_object.xsl";

scanWrapperDirectory($target_directory, 'structure', $path_to_xsl);

// For use with use with get_dir_name(), which is used inside XSLT.
$compound_obj_path = '';

function scanWrapperDirectory($target_directory, $structurefilename = 'structure', $path_to_xsl) {
    //  basenames to exclude.
    $exclude_array = array('..', '.DS_Store', 'Thumbs.db', '.');

    $stuffinwrapperdirectory = scandir($target_directory);
    foreach ($stuffinwrapperdirectory as $compoundObjectOrFile) {
        $objpath = $target_directory . DIRECTORY_SEPARATOR . $compoundObjectOrFile;
        if(!in_array($compoundObjectOrFile, $exclude_array) && is_dir($objpath)) {
           global $compound_obj_path;
           $compound_obj_path = $objpath;
           // subdirectories of wrapper directory will be compound object.
           // create a structure file for each.
           $structure_xml = compoundObjectStructureXML($objpath);

           // Apply XSLT
           $structure_xml = treeToCompound($path_to_xsl, $structure_xml);
           $structure_xml_output_file_path = $objpath . DIRECTORY_SEPARATOR 
                                            . $structurefilename . '.xml';
           file_put_contents($structure_xml_output_file_path, $structure_xml);
           
        }
        
    }
}


function treeToCompound($path_to_xsl, $tree_output_xml) {
    // Usage: php tree_to_compound.php tree_to_compound_object.xsl tree_output.xml

    $xsl = $path_to_xsl;
    // tree_output_xml is an xml string.
    $xml = $tree_output_xml;

    $xsl_doc = new DOMDocument();
    $xsl_doc->load($xsl);

    $xml_doc = new DOMDocument();
    $xml_doc->loadXML($xml);

    $xslt_proc = new XSLTProcessor();
    $xslt_proc->importStylesheet($xsl_doc);
    $xslt_proc->registerPHPFunctions();

    $output = $xslt_proc->transformToXML($xml_doc);

    return $output;
}

/**
 * Removes path segments leading up to the last segment.
 *
 * Called from within the XSLT stylesheet.
 */
function get_dir_name() {
    //global $input_dir;
    //global  $target_directory;
    global $compound_obj_path;
    $input_dir = $compound_obj_path;
    $dir_path = preg_replace('/(\.*)/', '', $input_dir);
    $dir_path = rtrim($dir_path, DIRECTORY_SEPARATOR);
    $base_dir_pattern = '#^.*' . DIRECTORY_SEPARATOR . '#';
    $dir_path = preg_replace($base_dir_pattern, '', $dir_path);
    $dir_path = ltrim($dir_path, DIRECTORY_SEPARATOR);
    echo $dir_path;
    return $dir_path;
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