# Veritracer's Common Trace Format

## Content types

* **id_field** : a 64-bits unsigned integer
* **double** : a double precision floating-point number
* **string** : a string
* **int** : a 64-bits signed integer

## Events

1. value
1. callpath
1. context

### value

A **value** event is composed of:
* **context** : id_field, usually the hash of the variable, it needs to be unique
* **parent** : id_field, an identifier for the callpath
* **mean** : double
* **max** : double
* **min** : double
* **std** : double, the standard deviation
* **median** : double
* **significant_digits** : double

### callpath

A **callpath** event is composed of:
* **parent** : id_field, the parent callpath's identifier of this callpath node
* **id** : id_field, the identifier of this callpath
* **name** : string

### context

A **context** event is composed of:
* **id** : a id_field, the id of this context, usually the hash of the variable, it link to the value events.
* **file** : string, the name of the source file
* **name** : string, the name of the variable
* **function** : string, the name of the function
* **line** : int, the line in the original source file
* **type** : string, the size in bytes of the variable