<?php
/**
 * @file
 */

/**
 * Class to handle campaign migration.
 */
class CampaignMigration extends DrupalNode7Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    $this->addSimpleMappings(array(
      'field_camp_link',
      'field_camp_settings',
      'field_camp_text_plain',
      'field_camp_weight',
      'field_camp_text_full'
    ));

    // Campaign body format
    $this->addFieldMapping('field_camp_text_full:format', 'field_camp_text_full:format')
         ->callbacks(array($this, 'mapFormat'));

    // Campaign images
    $this->addFieldMapping('field_camp_image', 'field_camp_image')
          ->sourceMigration('Files');
    $this->addFieldMapping('field_camp_image:file_class')->defaultValue('MigrateFileFid');
    $this->addFieldMapping('field_camp_image:file_replace', MigrateFile::FILE_EXISTS_REUSE);

    $this->addUnmigratedDestinations(array(
      'field_camp_pane_id',
    ));
  }

  public function prepareRow($row) {
    parent::prepareRow($row);

    // Replace nids.
    $node_map = $this->getNodeMap();
    $repcallback = function($matches) use ($node_map) {
      if (!empty($node_map[$matches[1]])) {
        return 'node/' . $node_map[$matches[1]];
      }
      return '';
    };
    $row->field_camp_link = preg_replace_callback('/node\/(\d+)/', $repcallback, $row->field_camp_link);

    // Process media tags.
    $file_map =
      db_select('migrate_map_files', 'mmf')
        ->fields('mmf', array('sourceid1', 'destid1'))
        ->execute()
        ->fetchAllKeyed();

    $repcallback2 = function($matches) use ($file_map) {
      return '"fid":' . $file_map[$matches[1]];
    };
    $row->field_camp_text_full[0] = preg_replace_callback('/"fid":"(\d+)"/', $repcallback2, $row->field_camp_text_full[0]);

    return TRUE;
  }

  public function prepare($entity, $row) {
    if (empty($row->field_camp_link)) {
      unset($entity->field_camp_link);
    }

    if (empty($row->field_camp_text_plain)) {
      unset($entity->field_camp_text_plain);
    }

    if (empty($row->field_camp_text_full)) {
      unset($entity->field_camp_text_full);
    }

    // This unset doesn't have effect, though ...
    unset($entity->field_camp_pane_id);

    // Save legacy nid.
    $entity->old_nid = $row->nid;
  }

  // Create campaign rules.
  public function complete($entity) {

    // @todo: use drupal_static to evade unnecessary DB access.
    $node_map = $this->getNodeMap();
    $term_map = $this->getTermMap();

    // Fetch rules from legacy DB.
    $db = Database::getConnection('default', $this->sourceConnection);
    $rules =
      $db->select('ding_campaign_rules', 'dcr')
        ->fields('dcr', array('nid', 'cid', 'rule', 'type'))
        ->condition('cid', $entity->old_nid)
        ->execute()
        ->fetchAll();

    foreach ($rules as $rule) {
      // Page rule
      if (!empty($rule->nid) && $rule->type == 'rule_page') {
        $nid = $node_map[$rule->nid];
        $repcallback = function($matches) use ($node_map) {
          if (!empty($node_map[$matches[1]])) {
            return '[nid:' . $node_map[$matches[1]] . ']';
          }
          return '';
        };
        $rule_value = preg_replace_callback('/\[nid:(\d+)\]/', $repcallback, $rule->rule);
        if ($rule_value == $rule->rule) {
          $rule_value = preg_replace_callback('/(\d+)/', $repcallback, $rule->rule);
        }
      }
      // Path rule.
      elseif ($rule->type == 'rule_path') {
        $nid = NULL;
        $repcallback = function($matches) use ($node_map) {
          if (!empty($node_map[$matches[1]])) {
            return 'node/' . $node_map[$matches[1]];
          }
          return '';
        };
        $rule_value = preg_replace_callback('/node\/(\d+)/', $repcallback, $rule->rule);
        // Path can't be empty.
        if (!$rule_value) {
          continue;
        }
      }
      // Taxonomy term rule.
      // NOTE: nid field is used to store term id.
      elseif (!empty($rule->nid) && $rule->type == 'rule_taxonomy') {
        $nid = $term_map[$rule->nid];
        $repcallback = function($matches) use ($term_map) {
          if (!empty($term_map[$matches[1]])) {
            return '[tid:' . $term_map[$matches[1]] . ']';
          }
          return '';
        };
        $rule_value = preg_replace_callback('/\[tid:(\d+)\]/', $repcallback, $rule->rule);
        if ($rule_value == $rule->rule) {
          $rule_value = preg_replace_callback('/(\d+)/', $repcallback, $rule->rule);
        }
      }
      // Generic rule
      else {
        $nid = NULL;
        $rule_value = $rule->rule;
      }

      $data = array(
        'nid' => $nid,
        'cid' => $entity->nid,
        'rule' => $rule_value,
        'type' => $rule->type,
      );

      // Skip the unused campaigns.
      db_insert('ding_campaign_rules')
        ->fields($data)
        ->execute();
    }
  }

  private function getNodeMap() {
    $node_map =
      db_select('migrate_map_event', 'mm')
        ->fields('mm', array('sourceid1', 'destid1'))
        ->execute()
        ->fetchAllKeyed();

    $node_map +=
      db_select('migrate_map_page', 'mm')
        ->fields('mm', array('sourceid1', 'destid1'))
        ->execute()
        ->fetchAllKeyed();

    $node_map +=
      db_select('migrate_map_news', 'mm')
        ->fields('mm', array('sourceid1', 'destid1'))
        ->execute()
        ->fetchAllKeyed();

    return $node_map;
  }

  private function getTermMap() {
    $term_map =
      db_select('migrate_map_category', 'mm')
        ->fields('mm', array('sourceid1', 'destid1'))
        ->execute()
        ->fetchAllKeyed();

    $term_map +=
      db_select('migrate_map_tags', 'mm')
        ->fields('mm', array('sourceid1', 'destid1'))
        ->execute()
        ->fetchAllKeyed();

    return $term_map;
  }
}
