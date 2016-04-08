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
    

    $pathinfo = pathinfo($this->compoundObjFilePath);
    $dir_name = $pathinfo['dirname'];
    $file_name = $pathinfo['filename'];    
    if($file_name != 'MODS'){
        $obj = $this->constructDatastream('OBJ', 'M');
        $obj->label = 'OBJ Datastream';
        // must infer mimetype from file.
        $mime_detector = new MimeDetect();
        $obj->mimetype = $mime_detector->getMimetype($this->compoundObjFilePath);
        $obj->setContentFromFile($this->compoundObjFilePath, FALSE);
        $this->ingestDatastream($obj);
    }
    
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
    
    
    
    // Add common relationship.
    $this->addRelationships();    
    
    // check if object is a parent wrapper object (will have it's own mods).
    // if it is not a parent wrapper object, infer parent from  compoundObjFilePath and
    // islandora_compound_batch table.
    // update the relationships accordingly - parent 
    
    $parent_pid = $this->inferParent($this->compoundObjFilePath);
    
    // Parent objects will return NULL from inferPerent method.
    if(!is_null($parent_pid)){
       $this->addRelationshipsForChild($parent_pid);
    } else {
       $this->addRelationshipsForParent(); 
    }

    // Indicate that this object is ready to actually go into Fedora.
    return ISLANDORA_BATCH_STATE__DONE;
  }
  
  
  /**
   * Determine if this is a child object based on the path and if so, return the parent PID.
   */
  private function inferParent($path) {
  
    // Assumes that the path to the DSID is ../pointer/dsid.EXT
    $pointer = basename(dirname($path));
    //echo $pointer . PHP_EOL;
        //->condition('pageptr', '%' . db_like($pageptr) . '%', 'LIKE')        
    $result = db_select('islandora_compound_batch', 'q')
        ->fields('q', array('parent_pid'))
        ->condition('q.pageptr', $pointer)
        ->execute()
        ->fetchAssoc();
    
    return $result['parent_pid'];
 
  }

  /**
   * Add basic relationships common to all objects.
   */
  public function addRelationships() {
    $this->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $this->preprocessorParameters['parent']);
  }
  
   /**
   * Add basic relationships for parent object.
   */
  public function addRelationshipsForParent() {
    // ToDo? Set model(s) based on extention to model map?
    // parent objects are compound
    $this->models = 'islandora:compoundCModel';
  }
  
  /** 
   *  Add parent-child relationships
   */
  public function addRelationshipsForChild($parent_pid){
    static $n = 0;
    $n = $n % 2;    
    $n = $n + 1;
    echo "FakeSequenceNum: $n" . PHP_EOL;
    $this->relationships->add(FEDORA_RELS_EXT_URI, 'isConstituentOf', $parent_pid);
    $escaped_pid = str_replace(':', '_', $parent_pid);
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, "isSequenceNumberOf$escaped_pid", $n, RELS_TYPE_PLAIN_LITERAL);
    //$this->relationships->add(ISLANDORA_RELS_EXT_URI, "isSequenceNumberOf$escaped_pid", $n, TRUE);
    /*
    <fedora:isConstituentOf rdf:resource="info:fedora/islandora:93"/>
    <islandora:isSequenceNumberOfislandora_93>1</islandora:isSequenceNumberOfislandora_93>
    */
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
    if($file_name == 'MODS') {
        $mods_file_path = $this->compoundObjFilePath;
    } else {
        $mods_file_path = $dir_name . DIRECTORY_SEPARATOR . 'MODS.xml';    
    }

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
