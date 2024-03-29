<?php
/**
 * @file
 */

/**
 * Class to handle basic node migration.
 */
class BaseNodeMigration extends DrupalNode7Migration {
  private $updateDashboard = FALSE;

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

    if (module_exists('ding_editorial_dashboard')) {
      module_load_include('install', 'ding_editorial_dashboard');
      $this->updateDashboard = TRUE;
    }
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
    $no_image_fid = 1255;

    $repcallback = function($matches) use ($file_map, $no_image_fid) {
      return '"fid":' . (isset($file_map[$matches[1]]) ? $file_map[$matches[1]] : "$no_image_fid");
    };
    $row->field_ding_body[0] = preg_replace_callback('/"fid":"(\d+)"/', $repcallback, $row->field_ding_body[0]);

    return TRUE;
  }

  /**
   * Ensure newly created nodes have proper settings in Dashboard.
   */
  public function complete($entity) {
    if ($this->updateDashboard && in_array($entity->type, array('ding_news', 'ding_event'))) {
      $db_node = new DashboardNode($entity->nid);
      $db_node->editorial_status = DASHBOARD_NODE_APPROVED;
      $db_node->save();
      $db_node->approve();
    }
  }
}
