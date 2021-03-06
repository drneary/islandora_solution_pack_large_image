<?php

/**
 * @file
 *
 * This file contains all the functions for image manipulation used in the large image solution pack.
 */

/**
 * Creates the derivatives for this content model type
 *
 * @param AbstractObject $object
 *   The object to add derivatives to.
 * @param bool $force
 *   Whether or not we should force the derivatives to be created, in the case
 *   that they already exist on the given object.
 *
 * @return boolean
 *   TRUE if all derivatives were created successfully, FALSE otherwise.
 */
function islandora_large_image_create_all_derivatives(AbstractObject $object, $force = TRUE) {
  if (!isset($object['OBJ'])) {
    drupal_set_message(t('Could not create image derivatives for %s. No image file was uploaded.', array('%s' => $object->id)), 'error');
    return FALSE;
  }

  $base_name = str_replace(':', '-', $object->id);
  $uploaded_file = islandora_large_image_get_uploaded_file($object, $base_name);

  if (! $uploaded_file)
    return FALSE;

  $mime_type = $object['OBJ']->mimeType;

  // drupal_set_message(t('islandora_large_image_create_all_derivatives: mime_type is %s', array('%s' => $mime_type)), 'error');


  if ($mime_type == 'image/tiff' or  $mime_type == 'image/tif'  ) {
    $ob  = TRUE;
    $jp2 = islandora_large_image_create_JP2_derivative($object, $uploaded_file, $base_name);
  // drupal file type doesn't currently support jp2k - will submit a bug report - for now, octet-stream will be assumed to be a jp2k:
  } elseif ( $mime_type == 'image/jp2' or $mime_type == 'application/octet-stream' ) {
    $ob = TRUE;
    $jp2 = islandora_large_image_add_datastream($object, 'JP2', $uploaded_file, 'image/jp2', t('Original JPEG 2000 "' . $object['OBJ']->label . '"'));
  } else {
    return FALSE;
  }

  $jpg = islandora_large_image_create_JPG_derivative($object, $uploaded_file, $base_name);
  $tn = islandora_large_image_create_TN_derivative($object, $uploaded_file, $base_name);

  return $ob && $jp2 && $jpg && $tn && file_unmanaged_delete($uploaded_file) && file_unmanaged_delete($jp2);

}


/**
 * Fetches the upload image file from Fedora, and saves it to a temp location.
 *
 * @param AbstractObject $object
 *  The object to fetch the uploaded image file from.
 * @param string $base_name
 *  The base name to use in creating temp files.
 *
 * @return string
 *   The file path to the temp file if successful, FALSE otherwise.
 */
function islandora_large_image_get_uploaded_file(AbstractObject $object, $base_name) {
  $mime_detector = new MimeDetect();
  $ext = $mime_detector->getExtension($object['OBJ']->mimeType);
  $filename = file_create_filename("{$base_name}_OBJ.{$ext}", 'temporary://');
  $object['OBJ']->getContent($filename);
  return $filename;
}

/**
 * Creates the JP2 derived datatstream from the uploaded image file.
 *
 * @param AbstractObject $object
 *   The object to add the derived datastream to.
 * @param string $uploaded_file
 *   The file path to the uploaded image file to derive from.
 * @param string $base_name
 *   The base name to use in creating temp files.
 * @param bool $force
 *   Whether or not we should force the derivatives to be created, in the case
 *   that they already exist on the given object.
 *
 * @return boolean
 *   TRUE if successful, FALSE otherwise.
 */
function islandora_large_image_create_JP2_derivative(AbstractObject $object, $uploaded_file, $base_name, $force = TRUE) {
  if ($force || !isset($object['JP2'])) {
    // create JP2 with kakadu
    module_load_include('inc', 'islandora_large_image', 'includes/utilities');
    $kakadu = variable_get('islandora_use_kakadu', !islandora_large_image_check_imagemagick_for_jpeg2000());
    $mime = strtolower($object['OBJ']->mimetype);
    if ($mime == 'image/jpeg' || $mime == 'image/jpg') {
      $kakadu = FALSE;
    }

    if ($kakadu) {
      $derivative_file = islandora_large_image_kdu_compress($uploaded_file, "temporary://{$base_name}_JP2.jp2"); // Create JP2
      if (!$derivative_file) {
        drupal_set_message(t("Kakadu failed. Trying ImageMagick ..."));
        // Force retry with ImageMagick if Kakadu has failed.
        $kakadu = FALSE; 
      }
    }
    if (!$kakadu) {
      $derivative_file = islandora_large_image_imagemagick_convert($uploaded_file, "temporary://{$base_name}_JP2.jp2", islandora_large_image_get_args());
    }
    if ($derivative_file === FALSE) {
      drupal_set_message(t('Failed to create JP2 derivative'), 'error');
      return FALSE;
    }
    islandora_large_image_add_datastream($object, 'JP2', $derivative_file, 'image/jp2', t('JPEG 2000'));
    return file_unmanaged_delete($derivative_file);
  }
  if (!$kakadu) {
    $derivative_file = islandora_large_image_imagemagick_convert($uploaded_file, "temporary://{$base_name}_JP2.jp2", islandora_large_image_get_args());
  }
  if ($derivative_file === FALSE) {
    drupal_set_message(t('Failed to create JP2 derivative'), 'error');
    return FALSE;
  }
  islandora_large_image_add_datastream($object, 'JP2', $derivative_file, 'image/jp2', t('JPEG 2000 derived from original TIFF'));
  $derivative_file;
}

/**
 * Creates the JPG derived datatstream from the uploaded image file.
 *
 * @param AbstractObject $object
 *   The object to add the derived datastream to.
 * @param string $uploaded_file
 *   The file path to the uploaded image file to derive from.
 * @param string $base_name
 *   The base name to use in creating temp files.
 * @param bool $force
 *   Whether or not we should force the derivatives to be created, in the case
 *   that they already exist on the given object.
 *
 * @return boolean
 *   TRUE if successful, FALSE otherwise.
 */
function islandora_large_image_create_JPG_derivative(AbstractObject $object, $uploaded_file, $base_name, $force = TRUE) {
  if ($force || !isset($object['JPG'])) {
    $args = array();
    $args[] = '-resize ' . escapeshellarg("600 x 800");
    $args[] = '-quality ' . escapeshellarg(variable_get('imagemagick_quality', 75));
    $derivative_file = islandora_large_image_imagemagick_convert($uploaded_file, "temporary://{$base_name}_JPG.jpg", $args);
    if ($derivative_file === FALSE) {
      drupal_set_message(t('Failed to create JPG derivative'), 'error');
      return FALSE;
    }
    islandora_large_image_add_datastream($object, 'JPG', $derivative_file, 'image/jpeg', t('Medium sized JPEG'));
    return file_unmanaged_delete($derivative_file);
  }
  return TRUE;
}

/**
 * Creates the TN derived datatstream from the uploaded image file.
 *
 * @param AbstractObject $object
 *   The object to add the derived datastream to.
 * @param string $uploaded_file
 *   The file path to the uploaded image file to derive from.
 * @param string $base_name
 *   The base name to use in creating temp files.
 * @param bool $force
 *   Whether or not we should force the derivatives to be created, in the case
 *   that they already exist on the given object.
 *
 * @return boolean
 *   TRUE if successful, FALSE otherwise.
 */
function islandora_large_image_create_TN_derivative(AbstractObject $object, $uploaded_file, $base_name, $force = TRUE) {
  if ($force || !isset($object['TN'])) {
    $args = array();
    $args[] = '-quality ' . escapeshellarg(variable_get('imagemagick_quality', 75));
    $args[] = '-resize ' . escapeshellarg("200 x 200");
    $derivative_file = islandora_large_image_imagemagick_convert($uploaded_file, "temporary://{$base_name}_TN.jpg", $args);
    if ($derivative_file === FALSE) {
      drupal_set_message(t('Failed to create TN derivative'), 'error');
      return FALSE;
    }
    islandora_large_image_add_datastream($object, 'TN', $derivative_file, 'image/jpeg', t('Thumbnail'));
    return file_unmanaged_delete($derivative_file);
  }
  return TRUE;
}

/**
 * Calls kdu_compress with the given arguments, or defaults if none are provided.
 *
 * @param string $src
 *   The source file to pass to kdu_compress
 * @param string $dest
 *   The destination file which kdu_compress will generate.
 * @param string $args
 *   Any additional arguments for kdu_compress, if not provided reasonable defaults will be used.
 *
 * @return
 *   Returns the newly generated file uri or FALSE if the conversion failed
 */
function islandora_large_image_kdu_compress($src, $dest, $args = NULL) {
  // Kinda weird logic, to avoid changing the source.
  $unlink_source = FALSE;

  // First, convert to (potentially uncompressed) TIFF file.
  if (!islandora_large_image_is_uncompressed_tiff($src)) {
    $c_args = array();

    // If the source is not a TIFF, or we have been instructed to uncompress
    //   TIFFs, make it so.
    if (!islandora_large_image_is_tiff($src) ||
      variable_get('islandora_large_image_uncompress_tiff', TRUE)) {
      $c_args[] = "-compress None";
    }
    $c_dest = file_create_filename(drupal_basename($src) . ".tif", 'temporary://');
    $src = islandora_large_image_imagemagick_convert($src, $c_dest, $c_args);
    $unlink_source = TRUE;
  }

  $args = isset($args) ? $args : '-rate 0.5 Clayers=1 Clevels=7 "Cprecincts={256,256},{256,256},{256,256},{128,128},{128,128},{64,64},{64,64},{32,32},{16,16}" "Corder=RPCL" "ORGgen_plt=yes" "ORGtparts=R" "Cblk={32,32}" Cuse_sop=yes'; // Default args.
  $src = drupal_realpath($src);
  $dest = drupal_realpath($dest);
  $ret = -1;
  $ret_palette = 0;
  $output = array();
  $kdu_home = variable_get('islandora_kakadu_url', FALSE);
  if ($kdu_home) {
    $kdu_compress = "$kdu_home/kdu_compress";
  } else {
    $kdu_compress = 'kdu_compress';
  }
  $command = "$kdu_compress -i \"$src\" -o \"$dest\" $args 2>&1";
  exec($command, $output, $ret);

  $output_string = implode(' ', $output);

  if (strpos($output_string, 'no_palette') !== FALSE) {
    // Warning was thrown re-create with the -no_palette flag so we don't end
    // up with a corrupted JP2.
    $args .= ' -no_palette';
    $command = "$kdu_compress -i \"$src\" -o \"$dest\" $args 2>&1";
    exec($command, $output, $ret_palette);
  }

  if ($ret != 0 || $ret_palette != 0) {
    $message = 'Kakadu failed to create derivatives<br/>Error: @ret<br/>Command: @command <br/>Output !output';
    $variables = array('@ret' => $ret, '@command' => $command, '!output' => implode('<br/>', $output));
    watchdog('islandora_large_image', $message, $variables, WATCHDOG_ERROR);
    return FALSE;
  }

  if ($unlink_source) {
    unlink($src);
  }

  return $dest;
}

/**
 * Calls kdu_expand with the given arguments, or defaults if none are provided.
 *
 * @param string $src
 *   The source file to pass to kdu_expand
 * @param string $dest
 *   The destination file which kdu_expand will generate.
 * @param string $args
 *   Any additional arguments for kdu_expand, if not provided reasonable defaults will be used.
 *
 * @return
 *   Returns the newly generated file uri or FALSE if the conversion failed
 */
function islandora_large_image_kdu_expand($src, $dest, $args = NULL) {
  $args = isset($args) ? $args : ''; // Default args.
  $src = drupal_realpath($src);
  $dest = drupal_realpath($dest);
  $ret = -1;
  $output = array();
  $kdu_home = variable_get('islandora_kakadu_url', FALSE);
  if ($kdu_home) {
    $kdu_expand = "$kdu_home/kdu_expand";
  } else {
    $kdu_expand = 'kdu_expand';
  }
  $command = "$kdu_expand -i \"$src\" -o \"$dest\" $args 2>&1";
  exec($command, $output, $ret);

  $output_string = implode(' ', $output);

  if ($ret != 0) {
    $message = 'Kakadu failed to create derivatives<br/>Error: @ret<br/>Command: @command <br/>Output !output';
    $variables = array('@ret' => $ret, '@command' => $command, '!output' => implode('<br/>', $output));
    watchdog('islandora_large_image', $message, $variables, WATCHDOG_ERROR);
    return FALSE;
  }
  
  return $dest;
}

/**
 * Calls imagemagick's convert command with the given arguments.
 *
 * @param string $src
 *   The source file uri to convert from.
 * @param string $dest
 *   The destination file uri to convert into.
 * @param int $width
 *   The source file to convert from.
 * @param int $height
 *   The destination file to convert into.
 *
 * @return string
 *   The destination file path if successful otherwise FALSE.
 */
function islandora_large_image_imagemagick_convert($src, $dest, $args) {
  $src = drupal_realpath($src) . '[0]';
  $dest = drupal_realpath($dest);
  $context = array(
    'source' => $src,
    'destination' => $dest,
  );
  drupal_alter('imagemagick_arguments', $args, $context);
  /**
   * To make use of ImageMagick 6's parenthetical command grouping we need to make the $source image the first parameter and $dest the last.
   * See http://www.imagemagick.org/Usage/basics/#cmdline
   */
  $command = escapeshellarg($src) . ' ' . implode(' ', $args) . ' ' . escapeshellarg($dest);
  $output = '';
  $ret = -1;
  if (_imagemagick_convert_exec($command, $output, $ret) !== TRUE) {
    $message = 'ImageMagick failed to create derivatives<br/>Error: @ret<br/>Command: @command<br/>Output: @output';
    $variables = array('@ret' => $ret, '@command' => $command, '@output' => $output);
    watchdog('islandora_large_image', $message, $variables, WATCHDOG_ERROR);
    return FALSE;
  }
  return $dest;
}

/**
 * Adds a datastream to an object.
 *
 * @param object $object
 *   The object to add a datastream to.
 * @param string $dsid
 *   The datastream ID to be added.
 * @param string $file
 *   The URI of the file to be added as the datastream content.
 * @param string $mimetype
 *   The mimetype of the file.
 * @param string $label
 *   The label for the datastream.
 *
 * @TODO: could this be a generic function? eg: islandora_add_datastream($object, $dsid, $file). ?
 */
function islandora_large_image_add_datastream(AbstractObject $object, $dsid, $file, $mimetype, $label) {
  $ds = $object->constructDatastream($dsid, 'M');
  $ds->label = $label;
  $ds->mimeType = $mimetype;
  $ds->setContentFromFile($file);
  try {
    $object->ingestDatastream($ds);
  } catch (exception $e) {
    drupal_set_message(t('@message', array('@message' => check_plain($e->getMessage()))), 'error');
    return FALSE;
  }
  return $file;
}

/**
 * Return preconfigured paramaters for ImageMagick JP2 creation
 * @return array
 *
 */
function islandora_large_image_get_args() {

  if (variable_get('islandora_lossless', FALSE)) {
    $args[] = " -define numrlvls=6";
    $args[] = " -define jp2:tilewidth=1024";
    $args[] = " -define jp2:tileheight=1024";
    $args[] = " -define jp2:rate=1.0";
    $args[] = " -define jp2:lazy";
    $args[] = " -define jp2:prg=rlcp";
    $args[] = " -define jp2:ilyrrates='0.015625,0.01858,0.0221,0.025,0.03125,0.03716,0.04419,0.05,0.0625, 0.075,0.088,0.1,0.125,0.15,0.18,0.21,0.25,0.3,0.35,0.4,0.5,0.6,0.7,0.84'";
    $args[] = " -define jp2:mode=int";
  }
  else {
    $args[] = "-define numrlvls=7";
    $args[] = "-define jp2:tilewidth=1024";
    $args[] = "-define jp2:tileheight=1024";
    $args[] = "-define jp2:rate=0.02348";
    $args[] = "-define jp2:prg=rpcl";
    $args[] = "-define jp2:mode=int";
    $args[] = "-define jp2:prcwidth=16383";
    $args[] = "-define jp2:prcheight=16383";
    $args[] = "-define jp2:cblkwidth=64";
    $args[] = "-define jp2:cblkheight=64";
    $args[] = "-define jp2:sop";
  }
  
  return $args;
}

/**
 * Uses Imagemagick's identify to examine the file in question.
 *
 * @param string $file
 *   A file-system path to the file in question.
 *
 * @return boolean
 *   A boolean indicating if the file contains a TIFF which is not compressed,
 *   (if it could be reencoded by the "free" version of Kakadu.
 */
function islandora_large_image_is_uncompressed_tiff($file) {
  $identify = islandora_large_image_get_identify();

  $file = drupal_realpath($file);

  $compression = exec(escapeshellcmd("$identify -format \"%C\" $file"));

  $compressed = (strtolower($compression) != 'none');

  return !$compressed && islandora_large_image_is_tiff($file);
}

/**
 * Uses Imagemagick's identify to determine if the file is a TIFF.
 *
 * @param string $file
 *   A file-system path to the file in question.
 *
 * @return boolean
 *   A boolean indicating if the file contains a TIFF.
 */
function islandora_large_image_is_tiff($file) {
  $identify = islandora_large_image_get_identify();

  $file = drupal_realpath($file);

  $codec = exec(escapeshellcmd("$identify -format \"%m\" $file"));

  $is_tiff = (strtolower($codec) == 'tiff');

  return $is_tiff;
}

/**
 * Attempt to get the "identify" executable...
 *
 * Assume convert is sanely named, and that identify can be obtained from the
 * same place.
 *
 * @return string
 *   The command to run for "identify".
 */
function islandora_large_image_get_identify() {
  // Get path for convert
  $convert = variable_get('imagemagick_convert', 'convert');

  // Replace "convert" with "identify"
  $identify = str_replace('convert', 'identify', $convert);

  return $identify;
}

