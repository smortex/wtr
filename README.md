# Work Time Recorder

A time tracking daemon to measure the time you spend working on different projects.

## Goals

* No user interaction to start / stop working on a project: I can't format my mind to add toil actions when I have something to do, so no "start" and "stop" buttons;
* Ability to work on multiple projects in parallel: I contribute to a lot of Free Software as a part of my day-to-day work, I want to track the time I spend on each of them;
* Ability to nest projects one inside another: one organization I am working for may own multiple projects, I want to track both the time spent working for this organization and on each project.

## How it works

The daemon `wtrd(1)` regularly walk through the user processes and check their working directory.  These working directories are compared to projects roots: if any process' working directory is in a project root, this project is considered active.

## Usage

Assuming the following directory tree for the user `~coyote` who wants to track the time spent on 3 projects, `A`, `B` and `C`, while also tracking the time spent for their employer `ACME` who owns the `A` and `B` projects:

```
~coyote
|-> ACME
|   |-> A
|   `-> B
`-> C
```

This user can create / edit their `wtrd(1)` configuration file with:

```sh-session
coyote@home ~ $ wtr edit
```

This will bring their editor where they can describe their hierarchy using ini-style syntax where each project's "friendly name" is a section containing a single `root` key with the path to the directory to consider the root of the project:

```ini
[A]
root=~/ACME/A

[B]
root=~/ACME/B

[C]
root=~/C

[ACME]
root=~/ACME
```

They then start the `wtrd(1)` daemon, and can use `wtr(1)` to query information on what is going on and generate reports.

```sh-session
coyote@home ~ $ wtr --help
```
