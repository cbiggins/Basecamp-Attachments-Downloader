<?php

require_once 'common.inc';
include 'api_paths.inc';

if (count($argv) < 2) {
  showUsage(); 
  exit;
}

$token = $argv[1];
$savepath = $argv[2];
$project = (isset($argv[3]) ? $argv[3] : FALSE);

if (!isset($savepath)) {
  showUsage();
  exit;
} else {

  // Make sure we have no slash on the end
  if (substr($savepath, -1) == '/') {
    $savepath = substr($savepath, 0, strlen($savepath) - 1);
  }

}

if (!isset($token)) {
  showUsage();
  exit;
}

if (!is_writable($savepath)) {
  sendMessage("ERROR: {$savepath} cannot be written to.");
  exit;
}

// New curl session ... 
$ch = newCurlSession($token);

// Get our project list ... 
$url = $root_path . $projects_list_path;
$result = curlRequest($ch, $url, $token);

if (!$result) {
  showCurlError($ch);
}

$resultsArray = xmlstr_to_array($result); 
$url = $root_path . $attachments_list_path;
$report = initReport(); 

$html = '';
foreach ($resultsArray['project'] as $projectArray) {
  
  // Do we need to look out for a specific project?
  if ($project) {
    if ($projectArray['name'] == $project) {
      // Process ... 
      getAttachments($ch, $projectArray, $url, $savepath, $report);
      $html .= '<a href="'.$projectArray['name'].'">'.$projectArray['name'].'</a><br />';
      break;

    }
  } else {
    getAttachments($ch, $projectArray, $url, $savepath, $report);
    $html .= '<a href="'.$projectArray['name'].'">'.$projectArray['name'].'</a><br />';
  }
}

file_put_contents($savepath . '/index.html', $html, FILE_APPEND);

