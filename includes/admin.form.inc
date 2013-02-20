<?php

/**
 * @file
 * Handles the display/submission of the admin settings form for this module.
 */

/**
 * Defines the admin settings form.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The Drupal form definition.
 */
function islandora_large_image_admin(array $form, array &$form_state) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora_large_image', 'includes/utilities');
  $get_default_value = function($name, $default) use(&$form_state) {
    return isset($form_state['values'][$name]) ? $form_state['values'][$name] : variable_get($name, $default);
  };
  $imagemagick_supports_jp2000 = islandora_large_image_check_imagemagick_for_jpeg2000();
  $kakadu = $get_default_value('islandora_kakadu_url', '/usr/bin/kdu_compress');
  $form = array(
    'islandora_lossless' => array(
      '#type' => 'checkbox',
      '#title' => t("Lossless Derivative Creation?"),
      '#default_value' => $get_default_value('islandora_lossless', FALSE),
      '#description' => t('Lossless derivative are of higher quality, but will adversely affect browser performance.'),
    ),
    'islandora_use_kakadu' => array(
      '#type' => 'checkbox',
      '#title' => t("Use Kakadu for image compression?"),
      '#disabled' => !$imagemagick_supports_jp2000,
      '#default_value' => $get_default_value('islandora_use_kakdu', !$imagemagick_supports_jp2000) && $imagemagick_supports_jp2000,
      '#description' => t("!kakadu offers faster derivative creation than the standard ImageMagick package. %magick_info", array(
                        '!kakadu' => l(t('kakadu'), 'http://www.kakadusoftware.com/'),
                        '%magick_info' => $imagemagick_supports_jp2000 ?
                        t('ImageMagick reports support of JPEG-2000s.') :
                        t('ImageMagick does not report support of JPEG-2000.'),
                      )),
    ),
    'islandora_large_image_uncompress_tiff' => array(
      '#type' => 'checkbox',
      '#title' => t('Uncompress TIFF files prior to creating JP2?'),
      '#description' => t('The version of Kakadu shipped with Djatoka does not ' .
                      'support compressed TIFFs; therefore, it is likely desirable to ' .
                      'uncompress the TIFF so Kakadu does not encounter an error. This will ' .
                      'not change the original TIFF stored in the OBJ datastream. Only ' .
                      'disable if you are completely sure!'),
      '#default_value' => $get_default_value('islandora_large_image_uncompress_tiff', TRUE),
      '#states' => array(
        'visible' => array(
          ':input[name="islandora_use_kakadu"]' => array('checked' => TRUE),
        ),
      ),
    ),
    'islandora_kakadu_url' => array(
      '#type' => 'textfield',
      '#title' => t("Path to kakadu"),
      '#default_value' => $kakadu,
      '#description' => t('Path to kakadu.<br/>!msg', array('!msg' => islandora_executable_available_message($kakadu))),
      '#prefix' => '<div id="kakadu-wrapper">',
      '#suffix' => '</div>',
      '#ajax' => array(
        'callback' => 'islandora_update_kakadu_url_div',
        'wrapper' => 'kakadu-wrapper',
        'effect' => 'fade',
        'event' => 'blur',
        'progress' => array('type' => 'throbber'),
      ),
      '#states' => array(
        'visible' => array(
          ':input[name="islandora_use_kakadu"]' => array('checked' => TRUE),
        ),
      ),
    ),
  );
  module_load_include('inc', 'islandora', 'includes/solution_packs');
  $form += islandora_viewers_form('islandora_large_image_viewers', 'image/jp2');
  $form['actions'] = array(
    '#type' => 'actions'
  );
  $form['actions']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset to defaults'),
    '#weight' => 1,
    '#submit' => array('islandora_large_image_admin_submit'),
  );
  return system_settings_form($form);
}

/**
 * Form submit
 */
function islandora_large_image_admin_submit($form, &$form_state) {
  $op = $form_state['clicked_button']['#id'];
  switch ($op) {
    case 'edit-reset':
      variable_del('islandora_large_image_viewers');
      break;
  }
}

function islandora_update_kakadu_url_div($form, $form_state) {
  return $form['islandora_kakadu_url'];
}