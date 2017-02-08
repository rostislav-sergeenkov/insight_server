<?php

/**
 * @file
 * Page callbacks defined in hook_menu for the Insight Server module.
 */

/**
 * Title callback for the project dashboard page.
 */
function insight_server_project_dashboard_title_callback($node) {
  return t('!project Dashboard', ['!project' => $node->title]);
}

/**
 * Builds Project Dashboard page.
 */
function insight_server_project_dashboard_form($form, &$form_state, $node) {
  $form['#tree'] = TRUE;
  $project_settings = $form_state['project_settings'] = insight_server_get_project_settings($node);

  foreach ($project_settings['sites_groups'] as $site_group) {
    $form[$site_group['machine_name']] = [
      '#type' => 'fieldset',
      '#title' => $site_group['machine_name'],
      '#collapsed' => FALSE,
      '#collapsible' => FALSE,
    ];

    $form[$site_group['machine_name']]['refresh'] = [
      '#type' => 'submit',
      '#value' => t('Refresh !environment', ['!environment' => $site_group['machine_name']]),
      '#submit' => ['insight_server_project_dashboard_form_refresh'],
      '#attributes' => [
        'data-site-group' => $site_group['machine_name'],
      ],
    ];

    $form[$site_group['machine_name']]['site_group'] = [
      '#type' => 'hidden',
      '#value' => $site_group['machine_name'],
    ];

    $form[$site_group['machine_name']]['table'] = [
      '#type' => 'markup',
      '#value' => insight_server_project_dashboard_get_report($project_settings, $site_group['machine_name']),
    ];
  }

  return $form;
}

function insight_server_project_dashboard_form_refresh($form, &$form_state) {
  $batch_data = [];
  $group_machine_name = $form_state['triggering_element']['#attributes']['data-site-group'];
  $project = $form_state['project_settings'];
  $sites_group_settings = $form_state['project_settings']['sites_groups'][$group_machine_name];

  foreach ($sites_group_settings['sites'] as $site_machine_name => $site) {
    $query_parameters = [
      'testSuites' => $sites_group_settings['test_suites'],
      'secretKey' => $sites_group_settings['key'],
    ];
    $batch_data[] = [
      $site['url'] . '/insight-agent/get?' . drupal_http_build_query($query_parameters),
      $sites_group_settings['statuses'],
      insight_server_get_site_variable_name($project['machine_name'], $sites_group_settings['machine_name'], $site_machine_name),
    ];
  }

  // @todo run Batch.
}

function insight_server_project_dashboard_get_report($project_settings, $group_machine_name) {
  $data = [];

  foreach ($project_settings['sites_groups'][$group_machine_name]['sites'] as $site_machine_name => $site) {
    $variable_name = insight_server_get_site_variable_name($project_settings['machine_name'], $group_machine_name, $site_machine_name);
    $data[$site_machine_name] = [
      'title' => $site['title'],
      'url' => $site['url'],
      'data' => variable_get($variable_name, []),
    ];
  }



  return [];
}

function insight_server_dashboard_form($form, &$form_state) {
  return [];
}

function insight_server_batch_result() {
  return [];
}