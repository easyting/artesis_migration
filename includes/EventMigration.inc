<?php
/**
 * @file
 */

/**
 * Class to handle basic node migration.
 */
class EventMigration extends BaseNodeMigration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    // Date field.
    $this->addSimpleMappings(array(
      'field_event_date' => 'field_event_date'
    ));

    $this->addUnmigratedDestinations(array(
      'field_materials',
      'field_audience',
      'field_genre',
      'field_material_type',
      'field_event_fee', 'field_event_fee:language'
    ));
  }

  /**
   * Handle adress field fetch from source.
   */
  public function prepare($entity, $row) {
    // Copy address values as is since we are migrating from D7 to D7.
    $source = (array)$row;
    $lng = $entity->language;
    foreach ($source as $field => $value) {
      // Country.
      if ($field == 'field_address') {
        $entity->field_address[$lng][0]['country'] = $value[0];
      }
      // Other subfields
      elseif (strpos($field, 'field_address') === 0) {
        list($fa, $subfield) = explode(':', $field);
        $entity->field_address[$lng][0][$subfield] = $value[0];
      }
    }
  }
}
