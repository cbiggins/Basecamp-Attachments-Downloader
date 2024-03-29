<?php

function sendReport($report) {

  $report_str = '';
  foreach ($report as $type => $count) {
    $report_str .= $type . ' = ' . $count . ' ';
  }

  sendMessage($report_str);
}

function sendMessage($msg) {
  print $msg . PHP_EOL;
}

function showUsage() {
  sendMessage("Usage: filesGetter.php token savepath [project]");
}

function newCurlSession($token) {
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_HTTPHEADER, "Accept: application/xml");
  curl_setopt($ch, CURLOPT_HTTPHEADER, "Content-Type: application/xml");
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
  curl_setopt($ch, CURLOPT_USERPWD, $token . ':X');
  return $ch;
}

function curlRequest($ch, $url) {
  curl_setopt($ch, CURLOPT_URL, $url);
  return curl_exec($ch);

}

function showCurlError($ch) {
  sendMessage(curl_error($ch));
}

function initReport() {
  return array('successful' => 0, 'failed' => 0);
}

function getAttachments($ch, $project, $url, $savepath, $report, $offset = 0) {

  $att_url = str_replace('{project_id}', $project['id'], $url);
  if ($offset > 0) {
    $att_url .= '?n='.$offset;
  }

  $result = curlRequest($ch, $att_url);

  if (!$result) {
    showCurlError($ch);
  }
  
  $attachArray = xmlstr_to_array($result);

  // Where to save these files
  $dir_path = $savepath . '/' . $project['name'];
  
  // Create our directory if it doesn't exist
  if (!file_exists($dir_path)) { 
    mkdir($dir_path, 0755, TRUE);
  }

  if ($offset == 0) {
    sendMessage("Processing " . $project['name']);  
  }

  $row_count = 0;

  $html = '';

  foreach($attachArray['attachment'] as $attachment) {
    
    $file_dest = $dir_path . '/' . $attachment['name'];

    $filename = $attachment['name'];

    if (file_exists($file_dest)) {
      // We'll change the filename so we can save dupes. Just append a timestamp before the extension
      $pathinfo = pathinfo($file_dest);
      $filename = $pathinfo['filename'] . '-' . time() . '.' . $pathinfo['extension']; 
      $file_dest = $dir_path . '/' . $filename;
    }

    $success = curlRequest($ch, $attachment['download-url']);
    if ($success) {
      file_put_contents($file_dest, $success);
      $html .= '<a href="'.$filename.'">'.$filename.'</a><br />';
      $report['successful']++;
    } else {
      sendMessage("Failed to retrieve " . $attachment['name']);
      $report['failed']++;
    }
  
    $row_count++;
  }

  if ($row_count == 100) {
    $offset = $offset + $row_count;
    getAttachments($ch, $project, $url, $savepath, $report, $offset);
  }
  else {
    file_put_contents($dir_path . '/index.html', $html, FILE_APPEND);
    sendReport($report);
  }
}


// The two functions below are from https://github.com/gaarf/XML-string-to-PHP-array

function xmlstr_to_array($xmlstr) {
  $doc = new DOMDocument();
  $doc->loadXML($xmlstr);
  return domnode_to_array($doc->documentElement);
}

function domnode_to_array($node) {
  $output = array();
  switch ($node->nodeType) {
   case XML_CDATA_SECTION_NODE:
   case XML_TEXT_NODE:
    $output = trim($node->textContent);
   break;
   case XML_ELEMENT_NODE:
    for ($i=0, $m=$node->childNodes->length; $i<$m; $i++) {
     $child = $node->childNodes->item($i);
     $v = domnode_to_array($child);
     if(isset($child->tagName)) {
       $t = $child->tagName;
       if(!isset($output[$t])) {
        $output[$t] = array();
       }
       $output[$t][] = $v;
     }
     elseif($v) {
      $output = (string) $v;
     }
    }
    if(is_array($output)) {
     if($node->attributes->length) {
      $a = array();
      foreach($node->attributes as $attrName => $attrNode) {
       $a[$attrName] = (string) $attrNode->value;
      }
      $output['@attributes'] = $a;
     }
     foreach ($output as $t => $v) {
      if(is_array($v) && count($v)==1 && $t!='@attributes') {
       $output[$t] = $v[0];
      }
     }
    }
   break;
  }
  return $output;
}
