<?php

/**
 * @file
 * Utility data and methods class.
 */

/**
 * Utilities class .
 */
class Utilities{

  /**
   * Extension to Islandora content model map
   * From: https://gist.github.com/mjordan/f1c8e84beb312e88c239.
   */
  public $extensionToContentModelMap = array();

  public function __construct() {
    $this->extensionToContentModelMap = array(
      'jpeg' => 'islandora:sp_basic_image',
      'jpg' => 'islandora:sp_basic_image',
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
      'zip' => 'islandora:binaryObjectCModel',
      'bin' => 'islandora:binaryObjectCModel',
      'docx' => 'islandora:sp_document',
      'pptx' => 'islandora:sp_document',
      'xlsx' => 'islandora:sp_document',
    );
  }

  /**
   * Returns an array of extensions in the Extension to Islandora content model map.
   */
  public function getObjExtensions() {
    return array_keys($this->extensionToContentModelMap);
  }

  /**
   * Checks if the $ext file extension is in the Extension to Islandora content model map.
   * Returns TRUE if it is and FALSE otherwise.
   */
  public function extInContentModelMap($ext) {
    return in_array($ext, $this->getObjExtensions());
  }

  public function getContentModelFromFileExt($fileExt) {
    return $this->extensionToContentModelMap[$fileExt];
  }

  /**
   * Parses optional --content_models drush param.
   *
   * @param string $fileExt
   *   The extension of the current child object's payload file.
   * @param string $map
   *   The extention::cmodelPID map.
   *
   * @return string/bool
   *   The PID of the content model to assign to the current child,
   *   FALSE if the mapping doesn't apply.
   */
  public function getContentModelFromParam($fileExt, $map) {
    list($targetExt, $cmodel) = explode('::', $map);
    if (strlen($targetExt)) {
      if ($targetExt == $fileExt) {
        return trim($cmodel);
      }
    }
  }

  /**
   * Applies an XSLT transform to an XML string.
   * Based off of
   * islandora_solution_pack_xml_apply_xslt in
   * https://github.com/mjordan/islandora_solution_pack_xml/blob/7.x/includes/utilities.inc .
   *
   * @param string $xslt
   *   An XSLT stylesheet.
   * @param string $input_xml
   *   An XML string.
   *
   * @return string
   *   The transformed XML.
   */
  function apply_xslt($xslt, $input_xml) {
    try {
      $xsl_doc = new DOMDocument();
      $xsl_doc->loadXML($xslt);
      $xml_doc = new DOMDocument();
      $xml_doc->loadXML($input_xml);
      $xslt_proc = new XSLTProcessor();
      $xslt_proc->importStylesheet($xsl_doc);
      $output_xml = $xslt_proc->transformToXML($xml_doc);
      return $output_xml;
    }
    catch (exception $e) {
      $success = array(
        'success' => FALSE,
        'messages' => array(
          array(
            'message' => t('Failed to apply XSL transform.'),
            'type' => 'watchdog',
            'severity' => WATCHDOG_ERROR,
          ),
        ),
      );
    }
  }

  /**
   * Recursively create XML string of directory structure/
   * Based on psuedo-code from http://stackoverflow.com/a/15096721/850828 .
   */
  private function directoryXML($directory_path) {

    //  basenames to exclude.
    $exclude_array = array('..', '.DS_Store', 'Thumbs.db', '.');

    $dir_name = basename($directory_path);
    $xml = "<directory name='" . $dir_name . "'>";

    $pathbase = pathinfo($directory_path, PATHINFO_BASENAME);

    $stuffindirectory = scandir($directory_path);

    foreach ($stuffindirectory as $subdirOrfile) {

      $subdirOrfilepath = $directory_path . DIRECTORY_SEPARATOR . $subdirOrfile;

      if (!in_array($subdirOrfile, $exclude_array) && is_file($subdirOrfilepath)) {
        $xml .= "<file name='" . $subdirOrfile . "' />";
      }

      if (!in_array($subdirOrfile, $exclude_array) && is_dir($subdirOrfilepath)) {
        $xml .= directoryXML($subdirOrfilepath);
      }

    }

    $xml .= "</directory>";

    return $xml;
  }

  public function createStructureXML($target_directory) {
    $xmlstring = "<tree>";
    $xmlstring .= $this->directoryXML($target_directory);
    $xmlstring .= "</tree>";
    $xml = new DOMDocument("1.0");
    $xml->loadXML($xmlstring);
    $xml->formatOutput = TRUE;
    return $xml->saveXML();
  }

}
