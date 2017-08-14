<?php
/*
  PHP FTP tail file read [2017/08/10]
  (c) SysTools 2017
  http://systools.losthost.org/
  https://github.com/systoolz/miscsoft/

  ftptail.php

  Tailing a file over FTP.
  PHP script which allows you to tail log files over FTP.

  Useful links:
  http://www.nsftools.com/tips/RawFTP.htm
*/

// sends command to FTP and retrive answer
function ftp_cmd_exec($fp, $cmd = '') {
  $s = '';
  if ($fp) {
    if (!empty($cmd)) {
      fwrite($fp, $cmd.chr(13).chr(10));
    }
    do {
      $l = fgets($fp, 1024);
      $s .= $l;
    } while((strlen($l) > 0) && (substr($l, 3, 1) == '-'));
  }
  return($s);
}

// retrive file from FTP via specified IP and port
function loadfile($serv, $port, $name) {
  $fp = @fsockopen($serv, $port, $errno, $errstr, 5);
  if ($fp) {
    stream_set_timeout($fp, 5);
    $fl = fopen($name, 'wb');
    if ($fl) {
      while (!feof($fp)) {
        $data = fread($fp, 1024*1024);
        fwrite($fl, $data, strlen($data));
      }
      fclose($fl);
    }
    fclose($fp);
  }
}

  // connection settings
  $l = array(
    // mandatory
    'file' => '/Upload/errorlog.log',
    'serv' => 'localhost',
     // optional
    'user' => 'anonymous',
    'pass' => 'a@example.com',
    'port' => intval(21),
    'tail' => floatval(1024*1024)
  );
  // from command line
  if (array_key_exists('argv', $_SERVER) && count($_SERVER['argv'])) {
    reset($l);
    for ($p = 1; $p < count($_SERVER['argv']); $p++) {
      // valid types - mostly for float filesize to use later
      $l[key($l)] = (
        is_float(current($l)) ? floatval($_SERVER['argv'][$p]) : (
          is_int(current($l)) ? intval($_SERVER['argv'][$p]) : $_SERVER['argv'][$p]
        )
      );
      // more arguments than expected
      if (next($l) === false) {
        break;
      }
    }
  }
  // create variables
  extract($l);
  // connect to FTP
  echo 'Connecting to "ftp://'.$serv.':'.$port.'/"...';
  $fp = @fsockopen($serv, $port, $errno, $errstr, 5);
  // something wrong
  if (!$fp) {
    echo 'failed'.PHP_EOL.'Error '.$errno.': '.trim($errstr).PHP_EOL;
    exit;
  }
  echo 'done'.PHP_EOL;
  // set read/write timeout
  stream_set_timeout($fp, 5);
  // read FTP server greetings message
  echo ftp_cmd_exec($fp);
  // login
  echo ftp_cmd_exec($fp, 'USER '.$user);
  echo ftp_cmd_exec($fp, 'PASS '.$pass);
  // get file size
  $size = ftp_cmd_exec($fp, 'SIZE '.$file);
  echo $size;
  // float because integer can't hold files more than 4 Gb
  $size = floatval(substr($size, 4));
  echo '>>>'.sprintf('%.0f', $size).PHP_EOL;
  // set binary transfer mode
  echo ftp_cmd_exec($fp, 'TYPE I');
  // switch to passive mode (usually it's the only allowed)
  $p = ftp_cmd_exec($fp, 'PASV');
  echo $p;
  // get IP and port for passive mode
  $l = array();
  preg_match('/([0-9]+,?){6}/', $p, $l);
  $l = explode(',', array_shift($l));
  $l = array_map('intval', $l);
  $p = array_pop($l);
  $p |= (array_pop($l) << 8);
  $s =implode('.', $l);
  // set file position to read from
  $tail = $size - $tail;
  // check if file bigger than required tail part (if not - get the whole file)
  if ($tail >= 1) {
    echo ftp_cmd_exec($fp, 'REST '.sprintf('%.0f', $tail));
  }
  // ask to retrive file
  echo ftp_cmd_exec($fp, 'RETR '.$file);
  // save file
  loadfile($s, $p, basename($file).'_part');
  echo ftp_cmd_exec($fp);
  // all done - quit
  echo ftp_cmd_exec($fp, 'QUIT');
  // close socket
  fclose($fp);
