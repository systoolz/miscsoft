<?php
/*
  Twitch.tv live streams playlist downloader
  (HTML5 and Flash players lags so much)

  (c) SysTools 2017-2021
  http://systools.losthost.org/
  https://github.com/systoolz/miscsoft/

  Windows native application "TwitchXP.exe" can be downloaded here:
  http://systools.losthost.org/?misc#twitchxp

  Changelog:
  2021.10.02 included JSON data validation
  2021.07.08 replaced fsockopen() with stream_socket_client() because of PHP 5.6.0+:
             fsockopen(): Peer certificate CN=`usher.ttvnw.net' did not match
             expected CN=`usher.twitch.tv' in twitchtv.php on line 41
  2019.01.06 add rawurlencode() since it was required for some args;
             removed replace_pattern() and replaced with sprintf()
  2018.05.23 Twitch.tv switched protocols from HTTP to HTTPS
  2017.01.08 first publice release

  References and documentation links for this code:
  https://www.johannesbader.ch/2014/01/find-video-url-of-twitch-tv-live-streams-or-past-broadcasts/

  AGDQ/SGDQ
  https://gamesdonequick.com/schedule
  https://player.twitch.tv/?volume=1&channel=gamesdonequick
*/

function get_page_from_web($link) {
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
      'GET '.$link.' HTTP/1.0'.$CRLF.
      'Host: '.$host.$CRLF.
      'Connection: Close'.$CRLF.
      $CRLF;
    fwrite($fp, $out);
    while (!feof($fp)) {
      $wp .= fgets($fp, 10 * 1024);
    }
    fclose($fp);
    $wp = substr($wp, strpos($wp, $CRLF.$CRLF) + 4);
  }
  return($wp);
}

function get_twich_playlist($channel) {
  $result = '';
  if (!empty($channel)) {
    $channel = rawurlencode($channel);
    $link = sprintf(
      'https://api.twitch.tv/api/channels/%s'.
      '/access_token?client_id=%s',
      $channel,
      // this client_id taken from the original Twitch.tv player file:
      // https://player.twitch.tv/vendor/TwitchPlayer.7cfe0f2e9d071ac72c5a539139bcedd4.swf
      'rp5xf0lwwskmtt1nyuee68mgd0hthrw'
    );
    $token = trim(get_page_from_web($link));
    if (!empty($token)) {
      $token = json_decode($token, true);
      if (
        (json_last_error() == JSON_ERROR_NONE) && (is_array($token)) &&
        (array_key_exists('token', $token)) && (array_key_exists('sig', $token))
      ) {
        $link = sprintf(
          'https://usher.twitch.tv/api/channel/hls/%s'.
          '.m3u8?player=twitchweb&token=%s'.
          '&sig=%s'.
          '&allow_audio_only=true&allow_source=true&type=any&p=%u',
          $channel,
          rawurlencode($token['token']),
          rawurlencode($token['sig']),
          time()
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
