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

    $node_map = $this->getNodeMap();

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
}
