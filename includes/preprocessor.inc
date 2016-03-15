<?php
/**
 * @file
 * Implementation of classes from the batch ingest framework.
 */
 
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
    
    echo PHP_EOL . "You've successfully run the drush command." . PHP_EOL;
    
    
    return $added;
  }
}