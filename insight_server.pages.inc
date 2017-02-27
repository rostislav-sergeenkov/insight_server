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

  // @todo check permissions.
  $form['update'] = [
    '#markup' => l(t('edit project settings'), 'node/' . arg(5) . '/edit',
      ['query' => ['destination' => 'admin/config/services/insight_server/dashboard/' . arg(5)]]),
  ];

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

  $form['#attached']['css'][] = drupal_get_path('module', 'insight_server') . '/insight_server.css';

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
    $agent_url = $site['url'] . '/' . INSIGHT_SERVER_AGENT_ENDPOINT . '?' . drupal_http_build_query($query_parameters);
    watchdog('insight_server', $agent_url);
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

// @todo get rid fo spans and pseudo elements in CSS - use CSS classes instead.
function insight_server_project_dashboard_get_report($project_settings, $group_machine_name) {
  $data = insight_server_project_dashboard_get_report_data($project_settings, $group_machine_name);

  if (empty(array_filter($data))) {
    return t('No previous test results found. Please click the button to get initial sanity check results.');
  }

  $markup = '';
  // @todo remove or refactor.
//  $markup = t('Checked: !interval by !user.', ['!interval' => $interval, '!user' => theme('username', [''])]);
  $user_info = [];
  $error_data = [];
  $success_data = [];
  $header = [t('property')];
  $table_attributes = [];

  // Splitting the results into 2 arrays: success and errors.
  foreach ($data as $site_machine_name => $results) {
    if (!empty($results['error'])) {
      $error_data[$site_machine_name] = $results;
    }
    else {
      $success_data[$site_machine_name] = $results;
    }
  }

  $rows = [];
  $checked_timestamp = 0;
  $uid = 0;
  $username = '';

  if (!empty($error_data)) {
    $rows_counter = 0;
    $rows[] = [t('Error')];
    $table_attributes = ['class' => ['drupal-insight-error-table']];

    foreach ($error_data as $error) {
      if (!empty($error)) {
        $header[] = l($error['site_title'], $error['host']) . ' (' . round($error['responseTime'] - $error['requestTime'], 3) . ' sec)';
        array_push($rows[0], ($error['code'] . ' ' . $error['error']));
        // @todo refactor.
        $checked_timestamp = round($error['responseTime']);
        $uid = $error['uid'];
        $username = $error['name'];

        $rows_counter++;
      }
    }
  }
  else {
    $sites = array_keys($success_data);
    $table_attributes = ['class' => ['drupal-insight-table']];

    // Build first 'Property' column using test labels of the first result.
    foreach ($success_data[$sites[0]]['results'] as $test_result) {
      array_push($rows, [t($test_result->label)]);
    }

    foreach ($success_data as $result_data) {
      if (!empty($result_data)) {
        $header[] = l($result_data['site_title'], $result_data['host']) . ' (' . round($result_data['responseTime'] - $result_data['requestTime'], 3) . ' sec)';
        $rows_counter = 0;
        // @todo refactor.
        $checked_timestamp = round($result_data['responseTime']);
        $uid = $result_data['uid'];
        $username = $result_data['name'];

        foreach ($result_data['results'] as $test_result) {
          if (!empty($test_result->url)) {
            // @todo check why destination doesn't work on external sites.
            $destination = drupal_get_destination();

            if (isset($destination['destination'])) {
              $destination['destination'] = $result_data['host'] . '/' . $destination['destination'];
            }

            $test_result->result = l($test_result->result, $result_data['host'] . '/' . $test_result->url,
              ['external' => TRUE, 'query' => $destination, 'attributes' => ['target' => '_blank']]);
          }

          array_push($rows[$rows_counter], '<span class="' . strtolower($test_result->status) . '">' . $test_result->result . '</span>');
          $rows_counter++;
        }
      }
    }
  }

  // @todo user object may not be present. Use user->name . ' ' . $user->mail from the response for such case.
  // @todo check on 'view user accounts' permission.
  $user_data = t('Last checked %interval by !user.', [
    '!user' => l($username, 'user/' . $uid),
    '%interval' => format_interval(REQUEST_TIME - $checked_timestamp) . ' ago',
  ]);

  $user_data = '<div class="user-data">' . $user_data . '</div>';

  $markup .= $user_data . theme('table', ['header' => $header, 'rows' => $rows, 'attributes' => $table_attributes]);

  return $markup;
}

function insight_server_dashboard_form($form, &$form_state) {
  $form['projects'] = [
    '#type' => 'select',
    '#options' => insight_server_get_projects_options(),
    '#default_value' => '',
  ];

  $form['#attached']['js'][] = drupal_get_path('module', 'insight_server') . '/insight_server.js';

  return $form;
}

/**
 * Returns array of projects as options.
 *
 * @return array
 */
function insight_server_get_projects_options() {
  global $base_path;
  $options = ['' => t('Please select the project')];
  $result = db_query('SELECT n.nid, n.title FROM {node} n WHERE n.type = :type ORDER BY n.title',
    [':type' => 'insight_server_project']);

  foreach ($result as $item) {
    $options[$base_path . 'admin/config/services/insight_server/dashboard/' . $item->nid] = $item->title;
  }

  return $options;
}
