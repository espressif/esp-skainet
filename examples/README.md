# Examples

This directory contains a range of examples in ESP-Skainet projects. These examples exist to demonstrate the functionality of ESP-Skainet and to provide the source codes that can be copied and adapted to during the development of your projects.

# Example Layout

The following two examples are currently available:

- get_started

  This example is a project template with four speech command words built-in.  
 Users can refer to the [readme](./get_started/README.md) of this example to understand the specific use and precautions of speech command recognition.

- garbage_classification

  In this example, we use wake word detection and speech commands recognition to build an off-line garbage sorting example, which can suggest the type of a garbage according to the present classification when a specific garbage name is identified. For example, if the board hears and identifies “卫生纸” (toilet paper), it plays “干垃圾” (residual waste), which indicates “卫生纸” (toilet paper) should be sorted as “干垃圾” (residual waste).

# Using Examples

The steps to build a new example is no different from building any other project:

* Follow the Getting Started instructions to create a "get_started" example.
* Navigate into the directory of the new example you'd like to build.
* Run `make menuconfig` to configure the example.
* Run `make` to build the example.
* Follow the instructions printed in the log to flash, or run the `make flash` command.

# Contributing Examples

If you have any examples you want to share with us or you think may interest us, please check the [Contributions Guide](https://esp-idf.readthedocs.io/en/latest/contribute/index.html) and get in touch with us.