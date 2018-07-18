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
  public function __construct(IslandoraTuque $connection, $compound_file_path, $batch_parameters) {
    parent::__construct(NULL, $connection->repository);

    $this->compoundObjFilePath = $compound_file_path;
    $this->preprocessorParameters = $batch_parameters;
  }

  /**
   * Assemble the base datastreams.
   */
  public function batchProcess() {
    // OBJ datastream.
    $batchset_id = $this->preprocessorParameters['set_id'];

    $pathinfo = pathinfo($this->compoundObjFilePath);
    $dir_name = $pathinfo['dirname'];
    $file_name = $pathinfo['filename'];
    if ($file_name == 'OBJ') {
      $this->ingestObjDatastream();
    }

    module_load_include('inc', 'islandora_compound_batch', 'includes/utilities');
    $cb_utilities = new Utilities();

    // MODS, then from it DC, datastreams.
    if ($mods = $this->getMods()) {

      $this->ingestDatastream($mods);

      $path_to_mods_to_dc_xsl = drupal_get_path('module', 'islandora_batch') . '/transforms/mods_to_dc.xsl';
      $mods_to_dc_xsl = file_get_contents($path_to_mods_to_dc_xsl);
      // Compound batch utilities class.
      $dc_content = $cb_utilities->apply_xslt($mods_to_dc_xsl, $mods->content);
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

    // Check if object is a parent wrapper object (will have it's own mods).
    // if it is not a parent wrapper object, infer parent from
    // compoundObjFilePath and islandora_compound_batch table.
    // update the relationships accordingly - parent.
    $compound_batch_row = $this->inferIslandoraCompoundBatchRow($this->compoundObjFilePath, $batchset_id);
    $parent_pid = $compound_batch_row['parent_pid'];
    $object_xpath = $compound_batch_row['object_xpath'];
    // Parent objects will return NULL from inferPerent method.
    if (!is_null($parent_pid)) {
      $sequence_number = $this->seqNumBasedOnXpath($object_xpath);
      $this->addRelationshipsForChild($parent_pid, $sequence_number);

      $file_extension = pathinfo($this->compoundObjFilePath, PATHINFO_EXTENSION);
      if ($this->preprocessorParameters['content_models']) {
        $maps = explode(',', $this->preprocessorParameters['content_models']);
        foreach ($maps as $map) {
          if ($content_model = $cb_utilities->getContentModelFromParam($file_extension, $map)) {
            $this->models = $content_model;
            // We've got our content model, let's not continue.
            break;
          }
          else {
            $content_model = $cb_utilities->getContentModelFromFileExt($file_extension);
            $this->models = $content_model;
          }
        }
      }
      else {
        $content_model = $cb_utilities->getContentModelFromFileExt($file_extension);
        $this->models = $content_model;
      }
    }
    else {
      $parent_obj_pattern = $dir_name . DIRECTORY_SEPARATOR . 'OBJ.*';
      $obj_file_list = glob($parent_obj_pattern);
      if (count($obj_file_list) === 1) {
        $file_extension = pathinfo($obj_file_list[0], PATHINFO_EXTENSION);
        $parent_content_model = $cb_utilities->getContentModelFromFileExt($file_extension);
        $this->models = $parent_content_model;
        $this->ingestObjDatastream($obj_file_list[0]);
      }
      else {
        $this->models = 'islandora:compoundCModel';
      }
      $this->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $this->preprocessorParameters['parent']);
    }

    // Indicate that this object is ready to actually go into Fedora.
    return ISLANDORA_BATCH_STATE__DONE;
  }

  /**
   * Infers place (sequence) in compound object level based on xpath.
   */
  private function seqNumBasedOnXpath($object_xpath) {

    // Example path for second sequence number cpd/page[2]
    $pattern = '%\[([0-9]*?)\]%';
    $result = preg_match($pattern, $object_xpath, $matches);
    // matches[0] will return square brackets.
    if ($result == 0) {
      // cpd/page will occur based on preprocessor code for
      // compound object with one child.
      // Requires sequence number 1 for thumbnails to appear
      // for parent object.
      return 1;
    }
    else {
      return $matches[1];
    }
  }

  /**
   * Infer the row that contains data for the given file (if any).
   */
  private function inferIslandoraCompoundBatchRow($path, $batchset_id) {

    // Assumes that the path to the DSID is
    // parent_directory/child_directory_name(if any)/dsid.EXT
    $parent_directory_name = basename(dirname(dirname($path)));
    $child_directory_name = basename(dirname($path));
    $child_content_value = $parent_directory_name . '/' . $child_directory_name;
    $result = db_select('islandora_compound_batch', 'q')
        ->fields('q')
        ->condition('q.child_content_value', $child_content_value)
        ->condition('q.batch_id', $batchset_id)
        ->execute()
        ->fetchAssoc();

    return $result;

  }

  /**
   * Implements abstract class defined in the main Batch class.
   */
  public function addRelationships() {
  }

  /**
   * Add parent-child relationships.
   */
  public function addRelationshipsForChild($parent_pid, $sequence_number) {
    /* (Grab the relationship predicate from the compound object solution pack setting,
        otherwise use fedora::isConstituentOf)
    <fedora:isConstituentOf rdf:resource="info:fedora/islandora:93"/>
    <islandora:isSequenceNumberOfislandora_93>1</islandora:isSequenceNumberOfislandora_93>
     */
    $rels_predicate = variable_get('islandora_compound_object_relationship', 'isConstituentOf');
    $this->relationships->add(FEDORA_RELS_EXT_URI, $rels_predicate, $parent_pid);
    $escaped_pid = str_replace(':', '_', $parent_pid);
    $this->relationships->add(ISLANDORA_RELS_EXT_URI, "isSequenceNumberOf$escaped_pid", $sequence_number, RELS_TYPE_PLAIN_LITERAL);

  }

  /**
   * Implements abstract class defined in the main Batch class.
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
    if ($file_name == 'MODS') {
      $mods_file_path = $this->compoundObjFilePath;
    }
    else {
      $mods_file_path = $dir_name . DIRECTORY_SEPARATOR . 'MODS.xml';
    }

    $marcxml_file_path = $dir_name . DIRECTORY_SEPARATOR . 'MARCXML.xml';

    if (file_exists($mods_file_path)) {
      $mods = $this->constructDatastream('MODS', 'M');
      $mods->mimetype = 'application/xml';
      $mods->label = 'MODS metadata';
      $mods->setContentFromFile($mods_file_path, FALSE);
      return $mods;
    }
    else if(file_exists($marcxml_file_path)) {
      // There is no MODS but there is MARCXML.
      $cb_utilities = new Utilities();
      $path_to_marc_to_mods_xsl = drupal_get_path('module', 'islandora_batch') . '/transforms/MARC21slim2MODS3-4.xsl';
      $marc_to_mods_xsl = file_get_contents($path_to_marc_to_mods_xsl);
      // Compound batch utilities class.
      $mods_content = $cb_utilities->apply_xslt($marc_to_mods_xsl, file_get_contents($marcxml_file_path));
      $mods = $this->constructDatastream('MODS', 'M');
      $mods->mimetype = 'application/xml';
      $mods->label = 'MODS metadata';
      $mods->setContentFromString($mods_content);
      return $mods;
    }
    else {
      // In the absence of a poper MODS or MARCXML file, generate a minimal one.
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

  /**
   * Creates an OBJ datastream.
   *
   * @param string $path_to_obj
   *   If present, the absolut path to the file to use as
   *   the OBJ datastream content.
   **/
  private function ingestObjDatastream($path_to_obj = NULL) {
    if ($this['OBJ']) {
      return;
    }
    $obj = $this->constructDatastream('OBJ', 'M');
    $obj->label = 'OBJ Datastream';
    // Must infer mimetype from file.
    $mime_detector = new MimeDetect();
    if (is_null($path_to_obj)) {
      $obj->mimetype = $mime_detector->getMimetype($this->compoundObjFilePath);
      $obj->setContentFromFile($this->compoundObjFilePath, FALSE);
    }
    else {
      $obj->mimetype = $mime_detector->getMimetype($path_to_obj);
      $obj->setContentFromFile($path_to_obj, FALSE);
    }
    $this->ingestDatastream($obj);
  }

}

