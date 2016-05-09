<?php
/**
 * @file
 * Implementation of classes from the batch ingest framework.
 */
 $path=drupal_get_path('module', 'islandora_compound_batch');
 include($path . '/includes/utilities.inc');
/**
 * Batch preprocessor class.
 */
class IslandoraCompoundBatchPreprocessor extends IslandoraBatchPreprocessor {
  // prefixed flag for outputing echo or print statements  
  protected $icbp_verbose = FALSE;
  /**
   * Function to get the OBJ XML files from the input directory.
   */
  public function preprocess() {
    
    if (strtolower($this->parameters['icbp_verbose']) == 'true') {
        $this->icbp_verbose = TRUE;
    }     
    
    //$input_path = $this->parameters['target'] . DIRECTORY_SEPARATOR . '*.xml';
    // Accumulate a list of the objects added to the queue.
    $added = array();
                
    $input_path = $this->parameters['target'];
    // Add batchset_id property for use later in ingest phase in object.inc
    //$batch_object->batchset_id = $this->getSetId();
    $this->parameters['set_id'] = $this->getSetId();
    
    $dir = new RecursiveDirectoryIterator($input_path);
    $iterator = new RecursiveIteratorIterator($dir);
    
    // compound batch utilities class   
    $cbUtilities = new Utilies;
    
    $files = array();
    foreach ($iterator as $fileinfo) {
        $filePath = $fileinfo->getPathname();
        $fileExtension = pathinfo($filePath, PATHINFO_EXTENSION);
        $isObjFile = $cbUtilities->extInContentModelMap($fileExtension);
        if ($fileinfo->isFile() && $isObjFile) {
            $batch_object = new IslandoraCompoundBatchObject($this->connection, $filePath, $this->parameters);
            //print_r($batch_object);
            //echo PHP_EOL;
            // Add the instances to the Islandora batch queue.
            $this->addToDatabase($batch_object);
            //$files[] = $filePath;
            $added[]= $batch_object;
        }
        if ($fileExtension == 'cpd') {
            
            // The structure file will be in the root directory of the compound object.
            // We need to create a parent wrapper object to associate the OBJ child objects too.
            // Along with the structure file in the root directory of the compound object
            // there should be a MODS file (or other metadata file) giving information about the 
            // object.  Associate this with the batch object.            
            
            $parentModsFilePath = dirname($filePath) . DIRECTORY_SEPARATOR . 'MODS.xml';
            //$parentPath = dirname($filePath) . DIRECTORY_SEPARATOR;
            //echo "Parent Mods Path: $parentModsFilePath" . PHP_EOL;
            $batch_object = new IslandoraCompoundBatchObject($this->connection, $parentModsFilePath, $this->parameters);
            // Parent wrapper object must be compound.
            $batch_object->models = 'islandora:compoundCmodel';
            //print_r($batch_object);
            //echo PHP_EOL;
            // Ad the instance to the Islandora batch queue.
            // Returns PID for parent object
            $compound_object_pid = $this->addToDatabase($batch_object);
            //echo "Return Value: $returnvalue" . PHP_EOL;

            $this->addCompoundStrcuture($filePath, $compound_object_pid);
            //$this->addCompoundStrcuture($filePath);
            
        }
        

    }
    
    return $added;
  }
  
  /**
   * Echos statement if icbp_verbose property is true (set using --icbp_verbose drush option)
   */
  public function icbpecho($string) {
     if ($this->icbp_verbose == TRUE) {
        echo $string;  
     }
  }  
  
  /**
   *  Parses CPD file and adds data to the islandora_compound_batch database table
   *  
   */
   public function addCompoundStrcuture($structure_file_path, $compound_object_pid) {
        static $object_num = 0;
        
        $batch_id = $this->getSetId();
        
        // Object ID is the batch set ID concatenated with the object number.
        $object_id = $batch_id . $object_num;
        
        // Increment 
        $object_num++;
        
        
        $doc = new DOMDocument();
        $doc->load($structure_file_path);
        
        $wanted_tags = array('node', 'page');
        foreach ($doc->getElementsByTagName('*') as $node) {
            if (in_array($node->nodeName, $wanted_tags)) {
              $this->icbpecho("Node name:\t" . $node->nodeName . "\n");
              if ($node->nodeName == 'page') {
                $xpath = new DOMXPath($doc);
                $pageptrs = $xpath->query('pageptr', $node);
                $pageptr = $pageptrs->item(0);
                $this->icbpecho("Page pointer:\t" . $pageptr->nodeValue . "\n");
                $pageptrValue = $pageptr->nodeValue;
              }
              $path = $node->getNodePath();
              $this->icbpecho("Path:\t\t$path\n");
              if ($node->nodeName == 'node') {
                $xpath = new DOMXPath($doc);
                $node_titles = $xpath->query('nodetitle', $node);
                $node_title = $node_titles->item(0);
                $this->icbpecho("Node title:\t" . $node_title->nodeValue . "\n");
              }
              $parentNode = $node->parentNode;
              $this->icbpecho("Parent path:\t" . $parentNode->getNodePath() . "\n\n");
              $parentNode = trim($parentNode->getNodePath());
              $insert_result = db_insert('islandora_compound_batch')
                ->fields(array(
                   'object_id' => $object_id,
                   'object_xpath' => $path,
                   'parent_xpath' => $parentNode,                   
                   'pageptr' => $pageptrValue,
                   'parent_pid' => $compound_object_pid,
                   'batch_id' => $batch_id            
                ))
                ->execute();
                
               $this->icbpecho("Insert Result: $insert_result" . PHP_EOL);
            }
            
        }       
        
    } // End of addCompoundStrcuture
  
}