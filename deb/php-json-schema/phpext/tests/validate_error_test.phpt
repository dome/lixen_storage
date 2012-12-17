<?php header('content-type:text/plain; charset=utf-8');
error_reporting(E_ALL); ?>
--TEST--
Check for jsonschema presence
--SKIPIF--

<?php if (!extension_loaded("jsonschema"))
    print "skip"; ?>

<?php
echo 'string', PHP_EOL;
$value = 'test s p a c e s string';
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo!($jsonSchema->validate('{"type":"string","format":"regex","pattern":"\/^[a-z.]+$\/i","minLength":0,"maxLength":2147483647}')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
1

<?php
echo 'number', PHP_EOL;
$value = 123.321;
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo!($jsonSchema->validate('{"type":"number","default":0,"minimum":0,"maximum":120,"exclusiveMinimum":0,"exclusiveMaximum":2147483647}')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
1

<?php
echo 'integer', PHP_EOL;
$value = 123;
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo!($jsonSchema->validate('{"type":"integer","default":0,"minimum":321,"maximum":2147483647,"exclusiveMinimum":0,"exclusiveMaximum":2147483647}')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
1

<?php
echo 'boolean', PHP_EOL;
$value = 12;
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo!($jsonSchema->validate('{"type":"boolean","default":false}')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
1

<?php
echo 'integer or boolean', PHP_EOL;
$value = "a string";
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo!($jsonSchema->validate('{"type":["boolean","integer"],"default":false}')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
$value = 123.321;
$jsonSchema = new JsonSchema(json_encode($value));
echo!($jsonSchema->validate('{"type":["boolean","integer"],"default":false}')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
2
1
2

<?php
echo 'object', PHP_EOL;
$value = new stdClass();
$value->name = 'a name';
$value->age = 30;
$value->height = "183";
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo!($jsonSchema->validate('{"type":"object","properties":{"name":{"type":"boolean"},"age":{"type":"integer","default":0,"minimum":20,"maximum":25,"exclusiveMinimum":20,"exclusiveMaximum":25},"height":{"type":"number","default":0,"minimum":0,"maximum":2147483647,"exclusiveMinimum":0,"exclusiveMaximum":2147483647}}}')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
1

<?php
echo 'array Map', PHP_EOL;
$value = array();
$value['name'] = 'a name';
$value['age'] = 23;
$value['height'] = 183.5;
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo!($jsonSchema->validate('{"type":"object","properties":{"name":{"type":"boolean"},"age":{"type":"integer","default":0,"minimum":20,"maximum":25,"exclusiveMinimum":20,"exclusiveMaximum":25},"height":{"type":"number","default":0,"minimum":0,"maximum":2147483647,"exclusiveMinimum":0,"exclusiveMaximum":2147483647}}}')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
1

<?php
echo 'array List', PHP_EOL;
$value = array();
$value[] = 'str A';
$value[] = 'str B';
$value[] = 'str C';
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo!($jsonSchema->validate('
            {
               "type":"array",
               "items":{
                  "type":"string",
                  "format":"regex","pattern":"/^[a-z0-9]+$/i",
                  "minLength":0,
                  "maxLength":2147483647
               }
            }')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
1

<?php
echo 'array List<Map>', PHP_EOL;
$value = array();
$value['users'][] = array('id' => 1, 'account' => 'userA');
$value['users'][] = array('id' => 3, 'account' => 'userB');
$value['users'][] = array('id' => 5, 'account' => 'userC');
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo!($jsonSchema->validate('{
                               "type":"object",
                               "properties":{
                                  "users":{
                                     "type":"array",
                                     "items":{
                                        "type":"object",
                                        "properties":{
                                           "id":{
                                              "type":"integer",
                                              "default":0,
                                              "minimum":0,
                                              "maximum":2147483647,
                                              "exclusiveMinimum":0,
                                              "exclusiveMaximum":2147483647
                                           },
                                           "account":{
                                              "type":"string",
                                              "minLength":0,
                                              "maxLength":3
                                           }
                                        }
                                     }
                                  }
                               }
                            }')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
1



<?php
echo 'array List<Map>', PHP_EOL;
$value = array();
$value['users'][] = array('id' => 1, 'account' => 'userA');
$value['users'][] = array('id' => 3, 'account' => 'userB');
$value['users'][] = array('id' => 5, 'account' => 'userC');
$jsonSchema = new JsonSchema(json_encode($value));
echo '--RESULT--', PHP_EOL;
echo!($jsonSchema->validate('{
                               "type":"object",
                               "properties":{
                                  "users":{
                                     "type":"array",
                                     "minItems":20,
                                     "maxItems":50,
                                     "items":{
                                        "type":"object",
                                        "properties":{
                                           "id":{
                                              "type":"integer",
                                              "default":0,
                                              "minimum":0,
                                              "maximum":2147483647,
                                              "exclusiveMinimum":0,
                                              "exclusiveMaximum":2147483647
                                           },
                                           "account":{
                                              "type":"string",
                                              "minLength":0,
                                              "maxLength":3
                                           }
                                        }
                                     }
                                  }
                               }
                            }')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
1

<?php
$userType = '
            {
                "id": "user",
                "description": "user info",
                "type": "object",
                "optional": true,
                "properties": {
                    "account": {"type": "boolean"},
                    "email": {"type": "string", "optional": true}
                }
            }';

echo 'array List<Map>', PHP_EOL;
$type = array();
$type['users'][] = array('account' => 'userA', 'email' => 'userA@example.com');
$type['users'][] = array('account' => 'userB', 'email' => 'userB@example.com');
$type['users'][] = array('account' => 'userC', 'email' => 'userC@example.com');
$jsonSchema = new JsonSchema(json_encode($type));
$jsonSchema->addType($userType);
echo!($jsonSchema->validate('
            {
               "type":"object",
               "properties":{
                  "users":{
                     "type":"array",
                     "items":{
                        "$ref":"user"
                     }
                  }
               }
            }')), PHP_EOL;
echo count($jsonSchema->getErrors()), PHP_EOL;
?>
--EXPECT--
1
1


