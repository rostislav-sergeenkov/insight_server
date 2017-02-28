<?php
/**
 * @file
 * insight_server.features.filter.inc
 */

/**
 * Implements hook_filter_default_formats().
 */
function insight_server_filter_default_formats() {
  $formats = array();

  // Exported format: Mail HTML.
  $formats['mail_html'] = array(
    'format' => 'mail_html',
    'name' => 'Mail HTML',
    'cache' => 1,
    'status' => 1,
    'weight' => 0,
    'filters' => array(
      'filter_autop' => array(
        'weight' => 0,
        'status' => 1,
        'settings' => array(),
      ),
      'filter_url' => array(
        'weight' => 0,
        'status' => 1,
        'settings' => array(
          'filter_url_length' => 72,
        ),
      ),
      'filter_htmlcorrector' => array(
        'weight' => 10,
        'status' => 1,
        'settings' => array(),
      ),
    ),
  );

  return $formats;
}
