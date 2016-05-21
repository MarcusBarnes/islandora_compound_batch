# Islandora Compound Batch

## Introduction

This module extends the Islandora batch framework so as to provide a Drush option to add compound items. Currently, only compound items that have a "flat" structure are supported.

The ingest is a three-step process:

* Generating a structure file for each compound object.
* Islandora Batch preprocessing: The data is scanned and a number of entries are created in the Drupal database.  There is minimal processing done at this point, so preprocessing can be completed outside of a batch process.
* Islandora Batch ingest: The data is actually processed and ingested. This happens inside of a Drupal batch.

The first step is accomplished by running a standalone PHP script on the directory containing your objects. The last two are drush commands similar to those provided by other Islandora Batch modules.

## Requirements

This module requires the following modules/libraries:

* [Islandora](https://github.com/islandora/islandora)
* [Islandora Batch](https://github.com/Islandora/islandora_batch)

# Installation

Install as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Configuration

There are no configuration options for this module.

### Usage

#### Arranging your content and generating structure files

To prepare your compound objects, arrange them in a directory structure so that each parent object is in its own directory beneath the input directory, and within each parent object, each child object is in its own subdirectory. Each parent should contain a MODS.xml file, which is a sibling to the child object directories. Each child object directory should contain a MODS.xml file and must contain a file corresponding to the OBJ datastream. This file must be named OBJ and use an extension that will determine the content model to use for the child object. A sample input directory is:

```
input_directory
├── parent_one
│   ├── first_child
│   │   ├── MODS.xml
│   │   └── OBJ.jp2
│   ├── second_child
│   │   ├── MODS.xml
│   │   └── OBJ.jp2
│   └── MODS.xml
└── parent_two
    ├── first_child
    │   ├── MODS.xml
    │   └── OBJ.jp2
    ├── second_child
    │   ├── MODS.xml
    │   └── OBJ.jp2
    └── MODS.xml
```

Once you have your content arranged in this way, you will need to generate a 'structure file' for your objects. To do this, run the `create_structure_files.php` script in this module's extras/scripts directory: `php create_strcutre_files.php path_to_directory_containing_compound_objects`. Running this script will add a `structure.xml` file to each parent object:

```
input_directory
├── parent_one
│   ├── first_child
│   │   ├── MODS.xml
│   │   └── OBJ.jp2
│   ├── second_child
│   │   ├── MODS.xml
│   │   └── OBJ.jp2
│   └── MODS.xml
│   └── structure.xml
└── parent_two
    ├── first_child
    │   ├── MODS.xml
    │   └── OBJ.jp2
    ├── second_child
    │   ├── MODS.xml
    │   └── OBJ.jp2
    └── MODS.xml
    └── structure.xml
```

If necessary, you can edit an object's `structure.xml` file to ensure that the children are in the order you want them to be in when they are ingested into the compound object in Islandora. The structure.xml files look like this:

```xml
<?xml version="1.0" encoding="utf-8"?>
<!--Islandora compound structure file used by the Compound Batch module. On batch ingest,
    'islandora_compound_object' elements become compound objects, and 'child' elements become their
    children. Files in directories named in child elements' 'content' attribute will be added as their
    datastreams. If 'islandora_compound_object' elements do not contain a MODS.xml file, the value of
    the 'title' attribute will be used as the parent's title/label.-->
<islandora_compound_object title="parent_one">
  <child content="first_child"/>
  <child content="second_child"/>
</islandora_compound_object>
```

#### Ingesting your prepared content into Islandora

The batch preprocessor is called as a drush script (see `drush help islandora_compound_batch_preprocess` for additional parameters):

`drush -v --user=admin islandora_compound_batch_preprocess --target=/path/to/input/directory --namespace=mynamespace --parent=namespace:some_collection`

This will populate the queue (stored in the Drupal database) with base entries.

The queue of preprocessed items is then processed by running the ingest command:

`drush -v --user=admin islandora_batch_ingest`

## Troubleshooting/Issues

Please open an issue in this Github repo's issue queue.

## Maintainers/Sponsors

* [Simon Fraser University Library](http://www.lib.sfu.ca/)

## Development

Pull requests are welcome, as are use cases and suggestions.

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)
