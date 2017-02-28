<?php

/**
 * @file
 * Batch functions and callbacks for the Insight Server module.
 */

/**
 * Batch callback - sends, processes and saves HTTP request results to Insight Agent.
 *
 * @param $site_title
 * @param $project
 * @param $site_group
 * @param $url
 * @param $variable
 * @param $context
 */
function insight_server_batch_process($site_title, $project, $site_group, $url, $variable, &$context) {
  $agent_response = insight_server_batch_send_request($site_title, $url);

  if (!isset($context['results']['project'])) {
    $context['results']['project'] = $project;
  }

  if (!isset($context['results']['site_group'])) {
    $context['results']['site_group'] = $site_group;
  }

  if (isset($agent_response['error'])) {
    $context['results']['errors'][] = t('Sanity check cannot be run on !url. Error: @error.',
      ['!url' => $site_title . ' (' . insight_server_get_hostname($url) . ')', '@error' => $agent_response['code'] . ' ' . $agent_response['error']]);
  }
  else {
    $context['results']['ok'][] = t('Sanity check is successfully completed on !url.',
      ['!url' => $site_title . ' (' . insight_server_get_hostname($url) . ')']);
  }

  variable_set($variable, $agent_response);

  // @todo remove. Added just to demonstrate possible network delays.
  sleep(1);
}

/**
 * Returns processed result of HTTP request to Insight Agent.
 *
 * @param $site_title
 * @param $url
 *
 * @return array
 */
function insight_server_batch_send_request($site_title, $url) {
  global $user;

  $response = [
    'uid' => $user->uid,
    'name' => $user->name,
    'mail' => $user->mail,
    'site_title' => $site_title,
    'host' => insight_server_get_hostname($url),
    'requestTime' => microtime(TRUE),
    'results' => [],
  ];

  // @todo leave the default value or define a constant.
  $agent_response = drupal_http_request($url, ['timeout' => 5]);

  if (!empty($agent_response->error)) {
    $response['error'] = $agent_response->error;
    $response['code'] = $agent_response->code;
  }
  else {
    $agent_response = json_decode($agent_response->data);
    $response['supports_test_suits'] = $agent_response->testSuits;

    if (!empty($agent_response->testSuites)) {
      // @todo refactor in the future to retain testSuites and not to have plain list of tests.
      foreach ($agent_response->testSuites as $suite) {
        foreach ($suite as $test_label => $test_results) {
          $response['results'][$test_label] = $test_results;
        }
      }
    }
  }

  $response['responseTime'] = microtime(TRUE);

  return $response;
}

/**
 * Batch finish callback.
 *
 * @param $success
 * @param $results
 * @param $operations
 */
function insight_server_batch_finished($success, $results, $operations) {
  if (!empty($results['errors'])) {
    foreach ($results['errors'] as $error) {
      drupal_set_message($error, 'error');
    }
  }

  if (!empty($results['ok'])) {
    foreach ($results['ok'] as $message) {
      drupal_set_message($message, 'status');
    }
  }

  // @todo get dynamic value instead of predefined constant.
  if (INSIGHT_SERVER_NOTIFICATION_ENABLED) {
    insight_server_send_notification($results['project'], $results['site_group']);
  }
}

/**
 * Sends email notification upon the completion of batch process.
 */
function insight_server_send_notification($project, $site_group) {
  // @todo get dynamic values instead of hardcoded values.
  module_load_include('inc', 'insight_server', 'insight_server.pages');

  $to = implode(', ', insight_server_get_team_emails($project));
  $body = insight_server_project_dashboard_get_report($project, $site_group['machine_name'], FALSE);

  $params = [
    'subject' => t('Sanity check results for @project @group',
      ['@project' => $project['title'], '@group' => $site_group['name']]),
    'body' => [$body],
  ];

  drupal_mail('insight_server', 'insight_server_notification', $to, language_default(), $params, variable_get('site_mail', ini_get('sendmail_from')));
}

/**
 * Returns array of team emails.
 *
 * @param $project
 *
 * @return array
 */
function insight_server_get_team_emails($project) {
  $emails = [];

  foreach ($project['team'] as $uid => $user) {
    $emails[$uid] = $user['mail'];
  }

  return $emails;
}
