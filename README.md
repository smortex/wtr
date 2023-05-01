# Work Time Recorder

A time tracking daemon to mesure the time you spend working on different projects.

## Goals

* No user interaction to start / stop working on a process: I can't format my mind to add toil actions when I have something to do;
* Ability to work on multiple projects in parallel: .
* Abilite to nest projects one inside another: one organization I am working for may have multiple projects I want to track.

## Proof of concept

The daemon `wtrd(1)` regulary walk through the user processes and check their working directory.  These working directories are compared to projects roots: if any process' working directory is in a project root, this project is considered active.
