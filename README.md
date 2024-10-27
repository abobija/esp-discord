# esp-discord

![CI](https://img.shields.io/github/actions/workflow/status/abobija/esp-discord/validate.yaml?branch=main&style=for-the-badge&logo=githubactions&logoColor=white) [![Component Registry](https://img.shields.io/github/v/release/abobija/esp-discord?sort=date&display_name=release&style=for-the-badge&logo=espressif&logoColor=white&label=Latest%20version)](https://components.espressif.com/components/abobija/esp-discord)

```
   __    ___  _ _                         
 /'__`\/',__)( '_`\                       
(  ___/\__, \| (_) )                      
`\____)(____/| ,__/'                      
     _       | |                        _ 
    ( ) _    (_)                       ( )
   _| |(_)  ___    ___    _    _ __   _| |
 /'_` || |/',__) /'___) /'_`\ ( '__)/'_` |
( (_| || |\__, \( (___ ( (_) )| |  ( (_| |
`\__,_)(_)(____/`\____)`\___/'(_)  `\__,_)

```

## Description

Library (component) for making Discord Bots on the ESP32, packaged as [ESP-IDF](https://github.com/espressif/esp-idf) component.

## Installation

This directory is an ESP-IDF component. Clone it (or add it as submodule) into `components` directory of the project, or add it as dependency:

```bash
idf.py add-dependency "abobija/esp-discord"
```

## Create project from example

> [!TIP]
> To find more interesting examples, go to [examples](examples) folder.

To run [`echo`](examples/echo) example, create it as follows:

```bash
idf.py create-project-from-example "abobija/esp-discord:echo"
```

Follow instructions from the project's README, then build and flash it as usual:

```bash
cd echo
idf.py build flash monitor
```

### Demo video

[![YouTube demo video](https://img.youtube.com/vi/p5qzRH2abvw/mqdefault.jpg)](https://www.youtube.com/watch?v=p5qzRH2abvw)

## Examples

More examples of using [esp-discord](https://github.com/abobija/esp-discord) can be found in separated [esp-discord-examples](https://github.com/abobija/esp-discord-examples) repository.

## Author

GitHub: [abobija](https://github.com/abobija)<br>
Homepage: [abobija.com](https://abobija.com)

## License

[MIT](LICENSE)
