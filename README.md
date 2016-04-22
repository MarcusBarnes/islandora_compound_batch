# Islandora Compound Batch [![Build Status](https://travis-ci.org/discoverygarden/islandora_newspaper_batch.png?branch=7.x)](https://travis-ci.org/discoverygarden/islandora_newspaper_batch)

## Introduction

This module extends the Islandora batch framework so as to provide a Drush option to add compound items.

The ingest is a two-step process:

* Preprocessing: The data is scanned and a number of entries are created in the
  Drupal database.  There is minimal processing done at this point, so preprocessing can
  be completed outside of a batch process.
* Ingest: The data is actually processed and ingested. This happens inside of
  a Drupal batch.

## Requirements

This module requires the following modules/libraries:

* [Islandora](https://github.com/islandora/islandora)
* [Tuque](https://github.com/islandora/tuque)
* [Islandora Batch](https://github.com/Islandora/islandora_batch)


# Installation

Install as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Configuration

N/A

### Usage

The base directory preprocessor can be called as a drush script (see `drush help islandora_compound_batch_preprocess` for additional parameters):

`drush -v --user=admin islandora_compound_batch_preprocess --target=/path/to/directory/ --namespace=mynamespace --parent=namespace:some_collection`

This will populate the queue (stored in the Drupal database) with base entries.

The queue of preprocessed items can then be processed:

`drush -v --user=admin islandora_batch_ingest`

## Troubleshooting/Issues

## Maintainers/Sponsors

This project has been sponsored by:

* [Simon Fraser University Library](http://www.lib.sfu.ca/)

## Development

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)