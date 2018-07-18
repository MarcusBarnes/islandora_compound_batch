<?php
/**
 * @file
 * Implementation of classes from the batch ingest framework.
 */

$path = drupal_get_path('module', 'islandora_compound_batch');
include $path . '/includes/utilities.inc';
/**
 * Batch preprocessor class.
 */
class IslandoraCompoundBatchPreprocessor extends IslandoraBatchPreprocessor {
  // Prefixed flag for outputing echo or print statements.
  protected $icbpVerbose = FALSE;
  /**
   * Function to get the OBJ XML files from the input directory.
   */
  public function preprocess() {
    if (strtolower($this->parameters['icbp_verbose']) == 'true') {
      $this->icbpVerbose = TRUE;
    }

    // Accumulate a list of the objects added to the queue.
    $added = array();

    $input_path = $this->parameters['target'];
    // Add batchset_id property for use later in ingest phase in object.inc
    // $batch_object->batchset_id = $this->getSetId();
    $this->parameters['set_id'] = $this->getSetId();

    $dir = new RecursiveDirectoryIterator($input_path);
    $iterator = new RecursiveIteratorIterator($dir);

    // Compound batch utilities class.
    $cb_utilities = new Utilities();
    $files = array();
    foreach ($iterator as $fileinfo) {
      $file_path = $fileinfo->getPathname();
      $file_dir = pathinfo($file_path, PATHINFO_DIRNAME);
      $file_grandparent_dir = pathinfo($file_dir, PATHINFO_DIRNAME);
      $file_extension = pathinfo($file_path, PATHINFO_EXTENSION);
      $file_name = pathinfo($file_path, PATHINFO_FILENAME);
      $file_name_with_extension = basename($file_path);
      $is_obj_file = $cb_utilities->extInContentModelMap($file_extension);
      if ($fileinfo->isFile() && $is_obj_file) {
        // A parent object that does not have the content model
        // 'islandora:compoundCmodel' may have a file named 'OBJ' in its
        // input directory. This OBJ file should be added to the parent
        // object as a datastream and not ingested as a separate object. To
        // prevent this, we compare the parent object's parent directory to the
        // batch job's input directory, since our input is organized such that
        // each parent object is in a child of the input directory (i.e., one
        // level below it). Check here and skip ingesting any objects whose
        // parent directoy is a direct child of the batch's input directory.
        if ($is_obj_file) {
          // Compare its grandparent directory to the batch's input directory.
          if ($file_grandparent_dir == rtrim($input_path, '/')) {
            // If they are the same, do not add this object to the queue since
            // the OBJ file is a datastream of the parent object, not its
            // own object.
            continue;
          }
        }
        $batch_object = new IslandoraCompoundBatchObject($this->connection, $file_path, $this->parameters);
        // Add the instances to the Islandora batch queue.
        $this->addToDatabase($batch_object);
        $added[] = $batch_object;
      }
      if ($file_name_with_extension == 'structure.xml') {
        // The structure file will be in the root directory of the compound
        // object.  We need to create a parent wrapper object to associate
        // the child objects too. Along with the structure file in the
        // root directory of the compound object there should be a MODS file
        // (or other metadata file) giving information about the object.
        // Associate this with the batch object. There may also be an OBJ file
        // that belongs to non-islandora:compoundCModel parent objects.
        $parent_mods_file_path = dirname($file_path) . DIRECTORY_SEPARATOR . 'MODS.xml';
        $batch_object = new IslandoraCompoundBatchObject($this->connection, $parent_mods_file_path, $this->parameters);

        $parent_obj_pattern = dirname($file_path) . DIRECTORY_SEPARATOR . 'OBJ.*';
        $obj_file_list = glob($parent_obj_pattern);
        if (count($obj_file_list) === 1) {
          $file_extension = pathinfo($obj_file_list[0], PATHINFO_EXTENSION);
          $parent_content_model = $cb_utilities->getContentModelFromFileExt($file_extension);
          // Parent wrapper object is determined by extension of any OBJ file present.
          $batch_object->models = $parent_content_model;
        }
        else {
          // Parent wrapper object is compound.
          $batch_object->models = 'islandora:compoundCmodel';
        }

        // Ad the instance to the Islandora batch queue.
        // Returns PID for parent object.
        $compound_object_pid = $this->addToDatabase($batch_object);
        $this->addStructureData($file_path, $compound_object_pid);
      }

    }

    return $added;
  }

  /**
   * Echos statement if icbpVerbose property is true.
   *
   * Set using --icbp_verbose drush option.
   */
  public function icbpecho($string) {
    if ($this->icbpVerbose == TRUE) {
      echo $string;
    }
  }

  /**
   * Parses the structure.xml file and adds data to the module's database table.
   */
  public function addStructureData($structure_file_path, $compound_object_pid) {
    static $object_num = 0;

    $batch_id = $this->getSetId();

    // Object ID is the batch set ID concatenated with the object number.
    $object_id = $batch_id . $object_num;

    // Increment.
    $object_num++;

    $doc = new DOMDocument();
    $doc->load($structure_file_path);

    /*
    <islandora_compound_object title="895">
    <child content="893"/>
    <child content="894"/>
    </islandora_compound_object>
     */

    $wanted_tags = array('child');
    foreach ($doc->getElementsByTagName('*') as $node) {
      if (in_array($node->nodeName, $wanted_tags)) {
        $this->icbpecho("Node name:\t" . $node->nodeName . "\n");
        if ($node->nodeName == 'child') {
          $xpath = new DOMXPath($doc);
          $child_contents = $xpath->query('@content', $node);
          $child_content = $child_contents->item(0);
          $this->icbpecho("Child content value:\t" . $child_content->nodeValue . "\n");
          $child_content_value = $child_content->nodeValue;
        }
        $path = $node->getNodePath();
        $this->icbpecho("Path:\t\t$path\n");

        $parent_node = $node->parentNode;
        $this->icbpecho("Parent path:\t" . $parent_node->getNodePath() . "\n\n");

        $parent_node = trim($parent_node->getNodePath());
        $insert_result = db_insert('islandora_compound_batch')
                    ->fields(array(
                        'object_id' => $object_id,
                        'object_xpath' => $path,
                        'parent_xpath' => $parent_node,
                        'child_content_value' => $child_content_value,
                        'parent_pid' => $compound_object_pid,
                        'batch_id' => $batch_id,
               ))->execute();

        $this->icbpecho("Insert Result: $insert_result" . PHP_EOL);
      }

    }

  } // End of addStructureData

}
