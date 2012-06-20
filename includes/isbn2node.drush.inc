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
      'drush isbn2node-import file' => 'Import ISBNs in file.',
      'drush isbn2node-import file FALSE' => 'Import ISBNs from file, but do not retrieve book covers.',
    ),
    'arguments' => array(
      'file'            => "The file to parse for ISBNs. The file is required to have one ISBN per line.",
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
function isbn2node_import($file, $get_images=TRUE) {
   // $fh = open($file);
   // foreach($line as readline($fh)) {
   //   preg_match($isbn_match, $line);
   $handle = fopen($file, "r");
   while($isbn = fgets($handle)) {
     // Fetch book data
     drush_print("trying ISBN: " + $isbn);
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
             // @TODO
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
             _isbn2node_make_node($isbn2book_data);
           }
         }
       } // if (!$result)
       else {
         drush_print("You already have a book with isbn: " . $isbn);
       }
     }
   }
}

