<?php
/**
 * @file
 * insight_server.features.inc
 */

/**
 * Implements hook_ctools_plugin_api().
 */
function insight_server_ctools_plugin_api($module = NULL, $api = NULL) {
  if ($module == "strongarm" && $api == "strongarm") {
    return array("version" => "1");
  }
}

/**
 * Implements hook_node_info().
 */
function insight_server_node_info() {
  $items = array(
    'insight_server_project' => array(
      'name' => t('Project'),
      'base' => 'node_content',
      'description' => '',
      'has_title' => '1',
      'title_label' => t('Project Title'),
      'help' => '',
    ),
  );
  drupal_alter('node_info', $items);
  return $items;
}
