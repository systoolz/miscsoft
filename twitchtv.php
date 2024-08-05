<?php
/*
  Twitch.tv live streams playlist downloader
  (HTML5 and Flash players lags so much)

  (c) SysTools 2017-2024
  http://systools.losthost.org/
  https://github.com/systoolz/miscsoft/

  Windows native application "TwitchXP.exe" can be downloaded here:
  http://systools.losthost.org/?misc#twitchxp

  Changelog:
  2024.08.05 updated JSON code in playlist request
  2022.12.28 old API has been removed, new implementation
  2021.10.02 included JSON data validation
  2021.07.08 replaced fsockopen() with stream_socket_client() because of PHP 5.6.0+:
             fsockopen(): Peer certificate CN=`usher.ttvnw.net' did not match
             expected CN=`usher.twitch.tv' in twitchtv.php on line 41
  2019.01.06 add rawurlencode() since it was required for some args;
             removed replace_pattern() and replaced with sprintf()
  2018.05.23 Twitch.tv switched protocols from HTTP to HTTPS
  2017.01.08 first publice release

  AGDQ/SGDQ
  https://gamesdonequick.com/schedule
  https://player.twitch.tv/?volume=1&channel=gamesdonequick
*/

function get_page_from_web($link, $data = '', $auth = array()) {
  $CRLF = chr(13).chr(10);
  $type = 'tcp://';
  $port = '80';
  if (!strcasecmp(substr($link, 0, 7), 'http://')) {
    $link = substr($link, 7);
  }
  if (!strcasecmp(substr($link, 0, 8), 'https://')) {
    $link = substr($link, 8);
    $type = 'ssl://';
    $port = '443';
  }
  $host = substr($link, 0, strpos($link, '/'));
  $link = substr($link, strpos($link, '/'));
  $wp = '';
  $context = stream_context_create(
    array('ssl' => array(
      'verify_peer' => false,
      'verify_peer_name' => false,
      'allow_self_signed' => true
    ))
  );
  $fp = stream_socket_client($type.$host.':'.$port, $errno, $errstr, 15, STREAM_CLIENT_CONNECT, $context);
  if ($fp) {
    $out =
      (empty($data) ? 'GET' : 'POST').' '.$link.' HTTP/1.0'.$CRLF.
      'Host: '.$host.$CRLF.
      (empty($auth) ? '' : implode($CRLF, $auth).$CRLF).
      (empty($data) ? '' :
        'Content-Length: '.strlen($data).$CRLF.
        'Content-Type: text/plain; charset=UTF-8'.$CRLF
      ).
      'Connection: Close'.$CRLF.
      $CRLF.
      $data;
    fwrite($fp, $out);
    while (!feof($fp)) {
      $wp .= fgets($fp, 10 * 1024);
    }
    fclose($fp);
    $fp = $CRLF.$CRLF;
    $out = strpos($wp, $fp);
    if ($out !== false) {
      $wp = substr($wp, $out + strlen($fp));
    }
  }
  return($wp);
}

function get_twich_playlist($channel) {
  $result = '';
  if (!empty($channel)) {
    $channel = rawurlencode($channel);
    // get clientId
    $token = get_page_from_web('https://www.twitch.tv/'.$channel);
    $auth = array();
    preg_match('/clientId\s*=\s*"([0-9a-z]+)/is', $token, $auth);
    $auth = array(
      'Client-ID: '.array_pop($auth), // required
      'Device-ID: '.substr(md5(strval(time())), 0, 16) // anything
    );
    // get query string
    $link = array();
    preg_match('/var\s+query\s*=\s*\'([^\']+)\'/is', $token, $link);
    $link = html_entity_decode(array_pop($link), ENT_NOQUOTES, 'UTF-8');
    $token = array(
      'operationName' => 'PlaybackAccessToken_Template',
      'query' => $link,
      'variables' => array(
        'isLive' => true,
        'login' => $channel,
        'isVod' => false,
        'vodID' => '',
        'playerType' => 'site',
        'platform' => 'web'
      )
    );
    // get token
    $token = trim(get_page_from_web('https://gql.twitch.tv/gql', json_encode($token), $auth));
    if (!empty($token)) {
      $token = json_decode($token, true);
      if (
        (json_last_error() == JSON_ERROR_NONE) && (is_array($token)) &&
        (array_key_exists('data', $token)) && (is_array($token['data'])) &&
        (array_key_exists('streamPlaybackAccessToken', $token['data']))
      ) {
        // usher.twitch.tv
        $link = sprintf(
          'https://usher.ttvnw.net/api/channel/hls/%s.m3u8?'.
          'acmb=%s'.
          '&allow_source=true'.
          '&fast_bread=true'.
          '&p=%u'.
          '&play_session_id=%s'.
          '&player_backend=mediaplayer'.
          '&playlist_include_framerate=true'.
          '&reassignments_supported=true'.
          '&sig=%s'.
          '&supported_codecs=avc1'.
          '&token=%s'.
          '&cdm=wv'.
          '&player_version=1.16.0',
          $channel,
          rawurlencode(base64_encode('{}')), // empty array
          time(),
          md5(strval(time())), // anything
          rawurlencode($token['data']['streamPlaybackAccessToken']['signature']),
          rawurlencode($token['data']['streamPlaybackAccessToken']['value'])
        );
        $result = get_page_from_web($link);
      }
    }
  }
  return($result);
}

/*
  // example of usage:
  file_put_contents('playlist.m3u', get_twich_playlist('gamesdonequick'));
*/
