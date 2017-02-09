<?php

/**
 * @file
 * Batch functions and callbacks for the Insight Server module.
 */

/**
 * Batch callback - sends, processes and saves HTTP request results to Insight Agent.
 *
 * @param $site_title
 * @param $url
 * @param $variable
 * @param $context
 */
function insight_server_batch_process($site_title, $url, $variable, &$context) {
  $agent_response = insight_server_batch_send_request($site_title, $url);

  if (isset($agent_response['error'])) {
    $context['results']['errors'][] = t('Cannot request test results from !url. Error: @error.',
      ['!url' => insight_server_get_hostname($url), '@error' => $agent_response['code'] . ' ' . $agent_response['error']]);
  }

  variable_set($variable, $agent_response);
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
}



