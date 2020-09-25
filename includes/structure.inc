<?php

/**
 * @file
 * Helper functions to create a "structure file".
 */

/**
 * For each compound object arranged under a root directory.
 *
 * Path_to_directory_containing_compound_objects\
 *    compound_object_1\
 *      child_1\
 *      child_2\
 *    compound_object_2\
 *      child_1\
 *      child_2\
 *      child_3\
 *    compound_object_3\
 *    [...]
 */

/**
 * Recursively scans the target directory.
 */
function scanWrapperDirectory($target_directory, $structurefilename = 'structure', $path_to_xsl) {
  // Generates the equivalent of the 'tree' command for each subdirectory.
  // And transforms the resulting XML into an Islandora structure file for each.
  trim($target_directory);
  if (!is_dir($target_directory)) {
    throw new Exeption("Please check that you have provided a full path to a directory as the input argument." . PHP_EOL);
  }
  if (!file_exists($path_to_xsl)) {
    throw new Exception("Cannot find the required XSLT file ($path_to_xsl)." . PHP_EOL);
  }
  // Filenames to exclude.
  $exclude_array = array('..', '.DS_Store', 'Thumbs.db', '.');
  $stuffinwrapperdirectory = scandir($target_directory);
  foreach ($stuffinwrapperdirectory as $compoundObjectOrFile) {
    $objpath = $target_directory . DIRECTORY_SEPARATOR . $compoundObjectOrFile;
    if (!in_array($compoundObjectOrFile, $exclude_array) && is_dir($objpath)) {
      // Subdirectories of wrapper directory will be compound object.
      // Create a structure file for each.
      $cpd_struct = compoundObjectStructureXML($objpath);
      // Apply XSLT.
      $structure_xml = treeToCompound($path_to_xsl, $cpd_struct);
      $structure_xml_output_file_path = $objpath . DIRECTORY_SEPARATOR
                                             . $structurefilename . '.xml';
      file_put_contents($structure_xml_output_file_path, $structure_xml);
    }
  }
}

/**
 * Applies xslt.
 */
function treeToCompound($path_to_xsl, $tree_output_xml) {
  $xsl = $path_to_xsl;
  // Where tree_output_xml is an xml string.
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
function get_dir_name($compound_obj_path) {
  global $compound_obj_path;
  $input_dir = $compound_obj_path;
  $dir_path = preg_replace('/(\.*)/', '', $input_dir);
  $dir_path = rtrim($dir_path, DIRECTORY_SEPARATOR);
  $base_dir_pattern = '#^.*' . DIRECTORY_SEPARATOR . '#';
  $dir_path = preg_replace($base_dir_pattern, '', $dir_path);
  $dir_path = ltrim($dir_path, DIRECTORY_SEPARATOR);
  return $dir_path;
}

/**
 * Recursively create XML string of directory/tree structure.
 *
 * Based on psuedo-code from http://stackoverflow.com/a/15096721/850828.
 */
function directoryXML($directory_path, $state = NULL) {
  // Basenames to exclude.
  $exclude_array = array('..', '.DS_Store', 'Thumbs.db', '.');
  $dir_name = basename($directory_path);
  if (!is_null($state)) {
    $xml = "<directory name='" . $state . "/" . $dir_name . "'>";
  }
  else {
    $xml = "<directory name='" . $dir_name . "'>";
  }
  $pathbase = pathinfo($directory_path, PATHINFO_BASENAME);
  $stuffindirectory = scandir($directory_path);
  sort($stuffindirectory, SORT_NATURAL);
  foreach ($stuffindirectory as $subdirOrfile) {
    $subdirOrfilepath = $directory_path . DIRECTORY_SEPARATOR . $subdirOrfile;
    if (!in_array($subdirOrfile, $exclude_array) && is_file($subdirOrfilepath)) {
      $xml .= "<file name='" . $subdirOrfile . "' />";
    }
    if (!in_array($subdirOrfile, $exclude_array) && is_dir($subdirOrfilepath)) {
      $state = $dir_name;
      $xml .= directoryXML($subdirOrfilepath, $state);
    }
  }
  $xml .= "</directory>";
  return $xml;
}

/**
 * Saves XML file.
 */
function compoundObjectStructureXML($dir_path) {
  $xmlstring = "<tree>";
  $xmlstring .= directoryXML($dir_path);
  $xmlstring .= "</tree>";
  $xml = new DOMDocument("1.0");
  $xml->loadXML($xmlstring);
  $xml->formatOutput = TRUE;
  return $xml->saveXML();
}
