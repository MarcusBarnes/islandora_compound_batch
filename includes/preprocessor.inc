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
  /**
   * Function to get the OBJ XML files from the input directory.
   */
  public function preprocess() {
    //$input_path = $this->parameters['target'] . DIRECTORY_SEPARATOR . '*.xml';
    // Accumulate a list of the objects added to the queue.
    $added = array();
        
    $input_path = $this->parameters['target'];
    
    $dir = new RecursiveDirectoryIterator($input_path);
    $iterator = new RecursiveIteratorIterator($dir);
    
    // compound batch utilities class   
    $cbUtilities = new Utilies;
    
    $files = array();
    foreach($iterator as $fileinfo) {
        $filePath = $fileinfo->getPathname();
        $fileExtension = pathinfo($filePath, PATHINFO_EXTENSION);
        $isObjFile = $cbUtilities->extInContentModelMap($fileExtension);
        if($fileinfo->isFile() && $isObjFile){
            $batch_object = new IslandoraCompoundBatchObject($this->connection, $filePath, $this->parameters);
            // Add the instances to the Islandora batch queue.
            $this->addToDatabase($batch_object);
            //$files[] = $filePath;
            $added[]= $batch_object;
        }
        if($fileExtension == 'cpd'){
            $this->addCompoundStrcuture($filePath);            
        }
    }
    
    return $added;
  }
  
  /**
   *  Parses CPD file and adds data to the islandora_compound_batch database table
   *  
   */
   public function addCompoundStrcuture($structure_file_path){
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
              echo "Node name:\t" . $node->nodeName . "\n";
              if ($node->nodeName == 'page') {
                $xpath = new DOMXPath($doc);
                $pageptrs = $xpath->query('pageptr', $node);
                $pageptr = $pageptrs->item(0);
                echo "Page pointer:\t" . $pageptr->nodeValue . "\n";
              }
              $path = $node->getNodePath();
              echo "Path:\t\t$path\n";
              if ($node->nodeName == 'node') {
                $xpath = new DOMXPath($doc);
                $node_titles = $xpath->query('nodetitle', $node);
                $node_title = $node_titles->item(0);
                echo "Node title:\t" . $node_title->nodeValue . "\n";
              }
              $parentNode = $node->parentNode;
              echo "Parent path:\t" .  $parentNode->getNodePath(). "\n\n";
              
              $insert_result = db_insert('islandora_compound_batch')
                ->fields(array(
                   'object_id' => $object_id,
                   'object_xpath' => $path,
                   'parent_xpath' => $parentNode,                   
                   //'sequence' =>
                   //'object_pid'
                   //'parent_pid'
                   'batch_id' => $batch_id            
                ))
                ->execute();
                
               echo "Insert Result: $insert_result" . PHP_EOL;
            }
            
        }
        
        
            
       
        
    }
  
}