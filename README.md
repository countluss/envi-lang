# envi

envi is a handmade scripting language for visual novels. This repository is its lexer and parser, written in C++23, with simdjson reading the JSON metadata. The `envi` program reads a `.envi` file, tokenizes it, and builds an abstract syntax tree.

Version 0.0.1 is a front end. It prints tokens and the tree. Playing a scene, loading art, and running game logic live outside this repository.

## A script

A script is a list of scenes plus one project block. Scenes hold the lines a player reads. The project block names the story and the scene it opens on.

```envi
@scene scene1 {
    @bg meadow

    @text "Frog" "Hello, I'm a frog."
    @text "The path splits ahead."
    @jump scene2
}

#project {
    "name": "Sample",
    "start": "scene1"
}
```

`sample.envi` and `sample2.envi` in this repository are longer examples of the same forms.

### Commands

Keywords are lowercase. Identifiers are ASCII letters and digits.

| Form | Where | Meaning |
| --- | --- | --- |
| `@scene name { ... }` | Top level | A scene. `name` is the scene id. |
| `@bg id` | Inside a scene | Show the background named `id`. |
| `@text "Name" "Line"` | Inside a scene | Dialogue spoken by `Name`. |
| `@text "Line"` | Inside a scene | Narration. The tree stores speaker id `0` and an empty name. |
| `@text id "Line"` | Inside a scene | Dialogue for a character id, with no display name on the node. |
| `@jump scene` | Inside a scene | Continue at the scene with that id. |
| `#project { ... }` | Top level | Story metadata. The block is JSON. |

`#project` needs two string fields:

```json
{
    "name": "Sample",
    "start": "scene1"
}
```

`name` is the story title. `start` is the id of the opening scene.

The lexer treats `@word` as a command, `#word` as a data block, `"..."` as a string, and a run of letters and digits as an identifier. After a `#` name it switches into JSON mode and takes everything up to the closing `}` as one JSON token, then returns to script mode. `{` and `}` in script mode are their own tokens. Newlines are tokens too, and the parser skips them.

Strings end at the next double quote. A `}` inside a `#project` block ends that block, so the JSON object stays flat: the two string fields above, with no nested objects.

## Build

You need a C++23 compiler and [Meson](https://mesonbuild.com/). simdjson 4.2.4 is already in `libraries/simdjson/` (upstream: [simdjson/simdjson](https://github.com/simdjson/simdjson)). The build does not fetch it.

```sh
meson setup build
meson compile -C build
```

The binary is `build/envi`. `meson install -C build` installs it.

## Usage

```text
envi -f <file> [-t] [-p]
```

| Flag | Effect |
| --- | --- |
| `-f <file>` | Script to read. Required. |
| `-t` | Print every token: type, text, and line. |
| `-p` | Parse and print the AST. Dialogue text is cut to four characters in this dump. |

`-t` and `-p` can be used together. With only `-f`, the tool reads and tokenizes the file, then exits.

```sh
./build/envi -f sample.envi -p
```

The script in [A script](#a-script) prints:

```text
ProgramNode
  SceneNode(scene1)
    BgNode(meadow)
    TextNode(,Frog,Hell...)
    TextNode(0,,The ...)
    JumpNode(scene2)
  ProjectDataNode(Sample,scene1)
```

`TextNode` fields are speaker id, display name, and the start of the line. An empty speaker id means the name was written inline. Speaker id `0` means narration.

A syntax error becomes an `ErrorNode` in the tree (message and line). Opening a missing file prints an error and exits with status 1.

## Layout

| Path | Role |
| --- | --- |
| `envi.cpp` | Command-line entry point. |
| `lexer.cpp`, `headers/lexer.h` | Scanner and token types. |
| `parser.cpp`, `headers/parser.h` | Parser and AST nodes. |
| `libraries/simdjson/` | Vendored simdjson, used for `#project`. |
| `meson.build` | Build description. C++23, version 0.0.1. |
| `sample.envi`, `sample2.envi` | Example scripts. |

## Pipeline

The lexer walks the source once. In script mode it emits commands, identifiers, strings, braces, and newlines. In JSON mode, entered by any `#` name, it emits one `JsonBlock` through the matching `}`.

The parser is recursive descent over that token stream. It accepts `@scene` and `#project` at the top level. Inside a scene it accepts `@bg`, `@text`, and `@jump`. Each construct becomes a node:

| Node | Fields |
| --- | --- |
| `ProgramNode` | The list of top-level nodes, in source order. |
| `SceneNode` | Scene name and its statements. |
| `BgNode` | Background id. |
| `TextNode` | Speaker id, display name, and line. |
| `JumpNode` | Target scene id. |
| `ProjectDataNode` | `name` and `start` from `#project`. |
| `ErrorNode` | Message and source line. |

`#project` is parsed with simdjson's on-demand API. The JSON is read once into a padded buffer and the two string fields are copied onto the node.

Scene ids may repeat. A `@jump` target is stored as written, whether or not that scene appears in the file.

## What 0.0.1 parses

Recognized today:

- `@scene`, `@bg`, `@text` (all three forms), `@jump`
- `#project` with string `name` and `start`

An unrecognized `@` command or top-level `#` block becomes an `ErrorNode`. Data blocks nested inside a scene are not implemented. There is no interpreter made yet.
