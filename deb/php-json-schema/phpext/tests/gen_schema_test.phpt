<?php header('content-type:text/plain; charset=utf-8');
error_reporting(E_ALL); ?>
--TEST--
Check for jsonschema presence
--SKIPIF--
<?php if (!extension_loaded("jsonschema"))
    print "skip"; ?>
<?php
echo 'test generate', PHP_EOL;
echo 'string', PHP_EOL;
$value = 'test string';
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo strcmp('{"type":"string","format":"regex","pattern":"\/^[a-z0-9]+$\/i","minLength":0,"maxLength":2147483647}', $jsonSchema->getSchema()), PHP_EOL;
?>
--EXPECT--
0

<?php
echo 'number', PHP_EOL;
$value = 123.321;
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo strcmp('{"type":"number","default":0,"minimum":0,"maximum":2147483647,"exclusiveMinimum":0,"exclusiveMaximum":2147483647}', $jsonSchema->getSchema()), PHP_EOL;
?>
--EXPECT--
0

<?php
echo 'integer', PHP_EOL;
$value = 123;
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo strcmp('{"type":"integer","default":0,"minimum":0,"maximum":2147483647,"exclusiveMinimum":0,"exclusiveMaximum":2147483647}', $jsonSchema->getSchema()), PHP_EOL;
?>
--EXPECT--
0
<?php
echo 'boolean', PHP_EOL;
$value = true;
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo strcmp('{"type":"boolean","default":false}', $jsonSchema->getSchema()), PHP_EOL;
?>
--EXPECT--
0

<?php
echo 'object', PHP_EOL;
$value = new stdClass();
$value->name = 'a name';
$value->age = 23;
$value->height = 183.5;
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo strcmp('{"type":"object","properties":{"name":{"type":"string","format":"regex","pattern":"\/^[a-z0-9]+$\/i","minLength":0,"maxLength":2147483647},"age":{"type":"integer","default":0,"minimum":0,"maximum":2147483647,"exclusiveMinimum":0,"exclusiveMaximum":2147483647},"height":{"type":"number","default":0,"minimum":0,"maximum":2147483647,"exclusiveMinimum":0,"exclusiveMaximum":2147483647}}}', $jsonSchema->getSchema()), PHP_EOL;
?>
--EXPECT--
0

<?php
echo 'array Map', PHP_EOL;
$value = array();
$value['name'] = 'a name';
$value['age'] = 23;
$value['height'] = 183.5;
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo strcmp('{"type":"object","properties":{"name":{"type":"string","format":"regex","pattern":"\/^[a-z0-9]+$\/i","minLength":0,"maxLength":2147483647},"age":{"type":"integer","default":0,"minimum":0,"maximum":2147483647,"exclusiveMinimum":0,"exclusiveMaximum":2147483647},"height":{"type":"number","default":0,"minimum":0,"maximum":2147483647,"exclusiveMinimum":0,"exclusiveMaximum":2147483647}}}', $jsonSchema->getSchema()), PHP_EOL;
?>
--EXPECT--
0

<?php
echo 'array List', PHP_EOL;
$value = array();
$value[] = 'a name';
$value[] = 23;
$value[] = 183.5;
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo strcmp('{"type":"array","minItems":0,"maxItems":20,"items":{"type":"string","format":"regex","pattern":"\/^[a-z0-9]+$\/i","minLength":0,"maxLength":2147483647}}', $jsonSchema->getSchema()), PHP_EOL;
?>
--EXPECT--
0

<?php
echo 'array List<Map>', PHP_EOL;
$value = array();
$value['users'][] = array('id' => 1, 'account' => 'userA');
$value['users'][] = array('id' => 3, 'account' => 'userB');
$value['users'][] = array('id' => 5, 'account' => 'userC');
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo strcmp('{"type":"object","properties":{"users":{"type":"array","minItems":0,"maxItems":20,"items":{"type":"object","properties":{"id":{"type":"integer","default":0,"minimum":0,"maximum":2147483647,"exclusiveMinimum":0,"exclusiveMaximum":2147483647},"account":{"type":"string","format":"regex","pattern":"\/^[a-z0-9]+$\/i","minLength":0,"maxLength":2147483647}}}}}}', $jsonSchema->getSchema()), PHP_EOL;
?>
--EXPECT--
0