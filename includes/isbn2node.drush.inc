<?php


/**
 * @file
 * Drush commands for isbn2node
 */

/**
 * Implementation of hook_drush_command().
 *
 * drush isbn2node-import <file-with-one-isbn-per-line>
 */
function isbn2node_drush_command() {
  $items['isbn2node-import'] = array(
    'callback' => 'isbn2node_import',
    'description' => dt('Import the ISBNs from a file.'),
    'aliases' => array('i2i'),
    'examples' => array(
      'drush isbn2node-import file' => 'Import ISBNs from file.',
      'drush isbn2node-import file uid' => 'Import ISBNs from file and make user uid the owner of the nodes.',
      'drush isbn2node-import file FALSE' => 'Import ISBNs from file but do not retrieve the book covers.',
    ),
    'arguments' => array(
      'file'            => "The file to parse for ISBNs. The file is required to have one ISBN per line.",
      'uid'             => "Optional. The uid of the user to set as content owner.",
      'get_images'      => "Optional. Boolean: whether to retrieve book-covers as well.",
    ),
  );
  return $items;
}

/**
 * Implementation of hook_drush_help().
 */
function isbn2node_drush_help($section) {
  switch ($section) {
    case 'drush:isbn2node-import':
      return dt("Import the ISBNs in the file.");
  }
}


/**
 * Import the ISBNs in the file.
 */
function isbn2node_import($file, $uid='', $get_images=TRUE) {
  $pwd = getenv("PWD") . "/";
  $handle = fopen($pwd . $file, "r");
  while($isbn = fgets($handle)) {
    // Fetch book data
    drush_print("trying ISBN: " . $isbn);
    $isbn = _isbn2book_normalize_isbn($isbn);
    if ($isbn) {
       // Check if ISBN is already in site database.
       $result = _isbn2node_isbn_exists($isbn);

       if (!$result) {
         $vals = _isbn2book_fetch_data($isbn);
         if (is_array($vals)) {
           $success = $vals['success'];
         }
         else {
           $success = $vals->success;
         }

         if (!$success) {
           drush_print("Didn't find any results on the ISBN: " . $isbn);
         }
         else {
           $isbn2book_data = _isbn2node_parse($vals, $isbn);
           if ($isbn2book_data) {
             // Insert ISBN
             $isbn2book_data['isbn2node_isbn'] = $isbn;

             // Fetch cover and create file object
             if ($get_images == TRUE) {
               if (isset($isbn2book_data['xisbn'])) {
                 $xisbn = $isbn2book_data['xisbn'];
               }
               else {
                 $xisbn = '';
               }
               $file = _isbn2node_save_cover($isbn, $xisbn);
               if ($file->fid) {
                 $isbn2book_data['isbn2node_cover'] = $file;
               }
             }

             // Create and save the node
             //   had to copy this to allow adding a node owner of $uid
             _isbn2node_drush_make_node($isbn2book_data, $uid);
           }
         }
       } // if (!$result)
       else {
         drush_print("You already have a book with isbn: " . $isbn);
       }
     }
   }
}


/*
 * Make and save a node based on a fields array
 * @param $fields array of node fields for isbn2node_book content type
 * @param $uid the user id of the content owner
 */
function _isbn2node_drush_make_node($fields, $uid) {
  $node = new stdClass();
  $node->title = $fields['title'];
  $node->type = 'isbn2node_book';
  $node->created = time();
  $node->changed= time();
  $node->status = 1;
  $node->promote = 1;
  $node->sticky = 0;
  $node->uid = $uid;
  $node->language = 'en';
  $node->timestamp = time();
  $node->revision = 0;

  // Remove title
  unset($fields['title']);

  // Handle the rest of the fields
  foreach ($fields as $field => $value) {
    if ($field == 'isbn2node_cover') {
      $cover = array('fid' => 0);
      // Handle special structure for image field
      foreach ($value as $k => $v) {
        $cover[$k] = $v;
      }
    $node->$field = array('und' => array(0 => $cover));
    }
    else {
      // Transfer normal field
      $node->$field = array('und' => array(0 => array('value' => $value)));
    }
  }

  node_submit($node);
  node_save($node);

  if (isset($node->nid)) {
    drupal_set_message(t('Created !title from ISBN @isbn.', array('!title' => l($node->title, 'node/' . $node->nid), '@isbn' => $fields['isbn2node_isbn'])));
  }
  else {
    drupal_set_message(t('Failed creating book from ISBN @isbn.', array('@isbn' => $fields['isbn2node_isbn'])));
  }

}
