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
      '#markup' => insight_server_project_dashboard_get_report($project_settings, $site_group['machine_name']),
    ];
  }

  return $form;
}

/**
 * Submit handler for "Refresh" buttons - run batch process.
 *
 * @todo break batch - 1 run per 1 test suit.
 */
function insight_server_project_dashboard_form_refresh($form, &$form_state) {
  $batch_operations = [];
  $project = $form_state['project_settings'];
  $group_machine_name = $form_state['triggering_element']['#attributes']['data-site-group'];
  $sites_group_settings = $form_state['project_settings']['sites_groups'][$group_machine_name];

  foreach ($sites_group_settings['sites'] as $site_machine_name => $site) {
    $query_parameters = [
      'testSuites' => $sites_group_settings['test_suites'],
      'secretKey' => $sites_group_settings['key'],
    ];
    $agent_url = $site['url'] . '/insight-agent/get?' . drupal_http_build_query($query_parameters);
    $variable_name = insight_server_get_site_variable_name($project['machine_name'], $sites_group_settings['machine_name'], $site_machine_name);
    $batch_operations[] = ['insight_server_batch_process', [$site['title'], $agent_url, $variable_name]];
  }

  $batch = array(
    'title' => t('Running sanity check...'),
    'progress_message' => t('Requested @current test results of total @total sites.'),
    'operations' => $batch_operations,
    'finished' => 'insight_server_batch_finished',
    'file' => drupal_get_path('module', 'insight_server') . '/insight_server.batch.inc',
  );

  batch_set($batch);
  batch_process();
}

function insight_server_project_dashboard_get_report_data($project_settings, $group_machine_name) {
  $data = [];

  foreach ($project_settings['sites_groups'][$group_machine_name]['sites'] as $site_machine_name => $site) {
    $variable_name = insight_server_get_site_variable_name($project_settings['machine_name'], $group_machine_name, $site_machine_name);
    $data[$site_machine_name] = variable_get($variable_name, []);
  }

  return $data;
}

function insight_server_project_dashboard_get_report($project_settings, $group_machine_name) {
  $data = insight_server_project_dashboard_get_report_data($project_settings, $group_machine_name);
  $markup = '';
  // @todo remove or refactor.
//  $markup = t('Checked: !interval by !user.', ['!interval' => $interval, '!user' => theme('username', [''])]);
  $user_info = [];
  $error_data = [];
  $success_data = [];

  // Splitting the results into 2 arrays: success and errors.
  foreach ($data as $site_machine_name => $results) {
    if (!empty($results['error'])) {
      $error_data[$site_machine_name] = $results;
    }
    else {
      $success_data[$site_machine_name] = $results;
    }
  }

  if (!empty($error_data)) {
    $header = [t('property')];
    $rows = [
      [t('Test Duration (sec)')],
      [t('Error')],
    ];

    foreach ($error_data as $error) {
      $header[] = l($error['site_title'], $error['host']);
      array_push($rows[0], round($error['responseTime'] - $error['requestTime'], 3));
      array_push($rows[1], ($error['code'] . ' ' . $error['error']));
    }

    $markup .= theme('table', ['header' => $header, 'rows' => $rows]);
  }

  return $markup;
}

function insight_server_dashboard_form($form, &$form_state) {
  return [];
}

function insight_server_batch_result() {
  return [];
}