<?php
/**
 * @file
 */

/**
 * Class to handle basic node migration.
 */
class BaseNodeMigration extends DrupalNode7Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    // Remove not-used default fields.
    $this->removeFieldMapping('body');
    $this->removeFieldMapping('body:language');
    $this->removeFieldMapping('body:summary');
    $this->removeFieldMapping('body:format');

    $this->addSimpleMappings(array(
      'field_ding_body',
      'field_ding_body:format',
      'field_ding_body:language',
      'field_ding_body:summary',
    ));

    $this->addFieldMapping('ding_content_tags', 'ding_content_tags')
         ->sourceMigration('Tags')
         ->arguments(array('source_type' => 'tid'));

    $this->addFieldMapping('field_category', 'field_category')
         ->sourceMigration('Category')
         ->arguments(array('source_type' => 'tid'));

    // Images
    $this->addFieldMapping('field_list_image', 'field_list_image')
          ->sourceMigration('Files');
    $this->addFieldMapping('field_list_image:file_class')->defaultValue('MigrateFileFid');
    $this->addFieldMapping('field_list_image:file_replace', MigrateFile::FILE_EXISTS_REUSE);

    $this->addFieldMapping('field_main_image', 'field_main_image')
         ->sourceMigration('Files');
    $this->addFieldMapping('field_main_image:file_class')->defaultValue('MigrateFileFid');
    $this->addFieldMapping('field_main_image:file_replace', MigrateFile::FILE_EXISTS_REUSE);

    // DNM
    $this->addUnmigratedSources(array(
      'field_ding_material',
      'group_audience',
    ));
  }

  /**
   * Process media tags.
   * @param array $row original data row.
   */
  public function prepareRow($row) {
    parent::prepareRow($row);

    $file_map =
      db_select('migrate_map_files', 'mmf')
        ->fields('mmf', array('sourceid1', 'destid1'))
        ->execute()
        ->fetchAllKeyed();

    $repcallback = function($matches) use ($file_map) {
      return '"fid":' . $file_map[$matches[1]];
    };
    $row->field_ding_body[0] = preg_replace_callback('/"fid":"(\d+)"/', $repcallback, $row->field_ding_body[0]);

    return TRUE;
  }
}
