<?php

/**
 * @file
 * Implementation of classes from the batch ingest framework.
 */

/**
 * Batch object class.
 */
class IslandoraCompoundBatchObject extends IslandoraBatchObject {
  protected $compoundObjFilePath;
  protected $preprocessorParameters;

  /**
   * Constructor.
   */
  public function __construct(IslandoraTuque $connection, string $compound_file_path, $batch_parameters) {
    parent::__construct(NULL, $connection->repository);

    $this->compoundObjFilePath = $compound_file_path;
    $this->preprocessorParameters = $batch_parameters;
  }

  /**
   * Assemble the base datastreams.
   */
  public function batchProcess() {
    // OBJ datastream.
    $obj = $this->constructDatastream('OBJ', 'M');
    $obj->label = 'OBJ Datastream';
    // must infer mimetype from file.
    $mime_detector = new MimeDetect();
    $obj->mimetype = $mime_detector->getMimetype($this->compoundObjFilePath);
    $obj->setContentFromFile($this->compoundObjFilePath, FALSE);
    $this->ingestDatastream($obj);
    
    // MODS, then from it DC, datastreams.    
    if ($mods = $this->getMods()) {
      
      $this->ingestDatastream($mods);

      module_load_include('inc', 'islandora_compound_batch', 'includes/utilities');
      $path_to_mods_to_dc_xsl = drupal_get_path('module', 'islandora_batch') . '/transforms/mods_to_dc.xsl';
      $mods_to_dc_xsl = file_get_contents($path_to_mods_to_dc_xsl);
      // compound batch utilities class   
      $cbUtilities = new Utilies;
      $dc_content = $cbUtilities->apply_xslt($mods_to_dc_xsl, $mods->content);
      $dc = $this->constructDatastream('DC', 'M');
      $dc->mimetype = 'application/xml';
      $dc->label = 'DC Record';
      $dc->setContentFromString($dc_content);
      $this->ingestDatastream($dc);

      // Get object label from dc:title.
      $dom = new DOMDocument();
      $dom->loadXML($dc_content);
      $xpath = new DOMXPath($dom);
      $xpath->registerNamespace('dc', 'http://purl.org/dc/elements/1.1/');
      $titles = $xpath->query("//dc:title");
      if ($titles->length > 0) {
        $this->label = $titles->item(0)->nodeValue;
      }
      else {
        $this->label = '[No object label available]';
      }
    }

    $this->addRelationships();
    
    // Indicate that this object is ready to actually go into Fedora.
    return ISLANDORA_BATCH_STATE__DONE;
  }

  /**
   * Add relationships.
   */
  public function addRelationships() {
    $this->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $this->preprocessorParameters['parent']);
    $this->models = 'islandora:compoundCmodel';
  }

  /**
   * Get a list of resources.
   */
  public function getResources() {
    return array();
  }

  /**
   * Creates a MODS datastream.
   *
   * If a file in the same directory as the OBJ file and matching its basename
   * plus '.MODS.xml' exists, populates the MODS datastream with that file;
   * if no such file exists, generates a minimal, templated MODS datastream.
   *
   * @return obj|FALSE
   *   A datastream object for the MODS document, or FALSE if no datastream
   *   was created.
   */
  private function getMods() {
    $pathinfo = pathinfo($this->compoundObjFilePath);
    $dir_name = $pathinfo['dirname'];
    $file_name = $pathinfo['filename'];
    $mods_file_path = $dir_name . DIRECTORY_SEPARATOR . 'MODS.xml';
    if (file_exists($mods_file_path)) {
      $mods = $this->constructDatastream('MODS', 'M');
      $mods->mimetype = 'application/xml';
      $mods->label = 'MODS metadata';
      $mods->setContentFromFile($mods_file_path, FALSE);
      return $mods;
    }
    else {
      // In the absence of a poper MODS file, generate a minimal one.
      $mods = $this->constructDatastream('MODS', 'M');
      $mods->content = <<<EOQ
<mods xmlns='http://www.loc.gov/mods/v3'>
  <titleInfo>
    <title>{$file_name}</title>
  </titleInfo>
</mods>
EOQ;
      $mods->mimetype = 'application/xml';
      $mods->label = 'MODS metadata';
      return $mods;
    }
    return FALSE;
  }



}
