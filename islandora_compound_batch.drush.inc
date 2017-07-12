<?php

/**
 * @file
 * Implementation of Drush hooks.
 */

/**
 * Implements hook_drush_command().
 */
function islandora_compound_batch_drush_command() {
  $items = array();

  $items['islandora_compound_batch_preprocess'] = array(
    'aliases' => array('icbp'),
    'description' => 'Preprocess compound objects into database entries.',
    /*'drupal dependencies' => array('islandora_batch', 'islandora_solution_pack_compound'),*/
    'drupal dependencies' => array('islandora_batch'),
    'examples' => array(
      format_string('drush -v icbp --user=admin --content_models=islandora:sp_large_image_cmodel --namespace=yul --parent=yul:F0433 --@target=/tmp/batch_ingest/ --icbp_verbose=TRUE',
      array(
        '@target' => DRUSH_VERSION >= 7 ? 'scan_target' : 'target',
      )),
    ),
    'options' => array(
      /*
      'type' => array(
        'description' => 'Either "directory" or "zip".',
        'required' => TRUE,
      ),*/
      'namespace' => array(
        'description' => 'The namespace for objects created by this command.  Defaults to namespace set in Fedora config.',
        'required' => FALSE,
      ),
      'content_models' => array(
        'description' => 'Defines the (single) content model to apply to child data files that have a specific extension. Use format "ext::cmodel_pid.',
        'value' => 'optional',
      ),
      'parent' => array(
        'description' => 'The collection to which the generated items should be added.  If "directory" and the directory containing the object description is a valid PID, it will be set as the parent. If this is specified and itself is a PID, all compound objects will be related to the given PID.',
        'required' => TRUE,
      ),
      'icbp_verbose' => array(
        'description' => 'Prints out additional information to the console if set to TRUE.',
        'value' => 'optional',
      ),
      'parent_relationship_uri' => array(
        'description' => 'The namespace URI of the relationship to the parent. Defaults to "info:fedora/fedora-system:def/relations-external#".',
        'value' => 'optional',
      ),
      'parent_relationship_pred' => array(
        'description' => 'The predicate of the relationship to the parent. Defaults to "isMemberOf".',
        'value' => 'optional',
      ),
//
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  // XXX: The target parameter became reserved in Drush 7 and above, for
  // backwards compatibility both will be supported. Not using
  // strict-option-handling (http://www.drush.org/en/master/strict-options) as
  // it requires manual argument parsing.
  if (DRUSH_VERSION >= 7) {
    $items['islandora_compound_batch_preprocess']['options']['scan_target'] = array(
      'description' => 'The target to directory or zip file to scan. Requires the full path to your archive from root directory. e.g. /var/www/drupal/sites/archive.zip',
      'required' => TRUE,
    );
  }
  else {
    $items['islandora_compound_batch_preprocess']['options']['target'] = array(
      'description' => 'The target to directory or zip file to scan. Requires the full path to your archive from root directory. e.g. /var/www/drupal/sites/archive.zip',
      'required' => TRUE,
    );
  }
  $items['islandora_compound_batch_prune_relationships'] = array(
    'aliases' => array('icbpr'),
    'description' => 'Removes rows from the islandora_compound_batch table that no longer have a batch associated with them.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );

  return $items;
}

/**
 * Implements hook_drush_command().
 */
function drush_islandora_compound_batch_preprocess() {
  // XXX: Due to how Drush bootstrapping works, the connection may be created
  // without credentials (when your site's front page is
  // 'islandora/object/some:object', for example). Resetting to ensure a new
  // connection gets created should fix it.
  drupal_static_reset('islandora_get_tuque_connection');
  $connection = islandora_get_tuque_connection();
  $parameters = array(
    'type' => drush_get_option('type'),
    'namespace' => drush_get_option('namespace'),
    'target' => DRUSH_VERSION >= 7 ? drush_get_option('scan_target') : drush_get_option('target'),
    'parent' => drush_get_option('parent', 'islandora:compoundCollection'),
    'content_models' => drush_get_option('content_models'),
    'icbp_verbose' => drush_get_option('icbp_verbose'),
    'parent_relationship_uri' => drush_get_option('parent_relationship_uri', 'info:fedora/fedora-system:def/relations-external#'),
    'parent_relationship_pred' => drush_get_option('parent_relationship_pred', 'isMemberOf'),
  );

  // Pass the preprocessor off to run.
  $preprocessor = new IslandoraCompoundBatchPreprocessor($connection, $parameters);
  $preprocessed = islandora_batch_handle_preprocessor($preprocessor);
  drush_log(t("SetId: @s", array('@s' => $preprocessor->getSetId())), "ok");
}

/**
 * Parent validation.
 */
function drush_islandora_compound_batch_preprocess_validate() {
  $parent = drush_get_option('parent');
  $object = islandora_object_load($parent);
  if (!$object) {
    return drush_set_error('This is not a valid parent', dt('The specified parent (@parent) was unable to be loaded.', array('@parent' => $parent)));
  }
  /*
  if (!in_array('islandora:compoundCModel', $object->models)) {
  return drush_set_error('Not a compound object',
  dt('The specified parent (@parent) is not a compound object.',
  array('@parent' => $parent)));
  }*/
}

/**
 * Implements hook_drush_command().
 */
function drush_islandora_compound_batch_prune_relationships() {
  // Delete all the rows from {islandora_compound_batch} that do not have
  // corresponding rows in {islandora_batch_set}.
  $result = db_query("DELETE icb FROM {islandora_compound_batch} icb LEFT JOIN {islandora_batch_set} o ON icb.batch_id = o.id WHERE o.id IS NULL");
  $num_deleted = $result->rowCount();
  drush_log(t("Deleted @n rows from islandora_compound_batch", array('@n' => $num_deleted)), "ok");
}
