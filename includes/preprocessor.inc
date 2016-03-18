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
    }
    
    return $added;
  }
}