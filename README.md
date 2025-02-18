# libget
[![gh actions](https://img.shields.io/github/actions/workflow/status/fortheusers/libget/main.yml?style=flat-square)](https://github.com/fortheusers/libget/actions/workflows/main.yml) [![gitlab ci](https://gitlab.com/4TU/libget/badges/master/pipeline.svg?style=flat-square)](https://gitlab.com/4TU/libget/pipelines) [![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://opensource.org/licenses/GPL-3.0)

get is a mini package manager that is used as the backend to [Homebrew App Store](http://github.com/vgmoose/hb-appstore) to allow more formal package management.

## Why?
**libget** is intended for use in smaller systems, such as homebrew'd video game consoles, where there is code execution but not a full stack of tools that package managers usually utilize, such as shell scripts or external libraries.

For documentation, metadata syntax, and repo setup, see [the wiki](https://github.com/vgmoose/get/wiki).

Versions of the get client 2.0.0 and after can support non-libget repo formats. See the "Example Local Repo JSONs" file below for how to specify other repo types.

## Usage
### Loading local repos
A "repos.json" file should be present on the local machine, in the same directory as the `get` binary. See [repos.json](https://github.com/vgmoose/get/blob/master/.get/repos.json) for what this file should look like. If this file does not exist, depending on the use case (such as on the Wii U) a default repos.json should automatically be generated.

#### Example Local Repo JSONs
Below are three example repos. The first two are libget repos used by the Switch and WiiU homebrew appstores, and the third and fourth are OSC / Universal-DB repos used by the Wii / 3DS.

<details>
  <summary>WiiU Homebrew CDN (libget Repo)</summary>
    
  ```javascript
  {
    "repos": [{
       "name":"WiiU ForTheUsers Repo",
       "url":"https://wiiu.cdn.fortheusers.org",
       "type":"get",
       "enabled":true
    }]
  }
  ```
</details>

<details>
  <summary>Switch Homebrew CDN (libget Repo)</summary>
    
  ```javascript
  {
    "repos": [{
       "name":"Switch ForTheUsers Repo",
       "url":"https://switch.cdn.fortheusers.org",
       "type":"get",
       "enabled":true
    }]
  }
  ```
</details>

<details>
  <summary>Wii Open Shop Channel (OSC Repo) (WIP)</summary>
    
  ```javascript
  {
    "repos": [{
       "name":"Wii OSC Repo",
       "url":"https://hbb1.oscwii.org",
       "type":"osc",
       "enabled":true
    }]
  }
  ```
</details>


<details>
  <summary>3DS Universal-DB (Unistore Repo) (WIP)</summary>
    
  ```javascript
  {
    "repos": [{
       "name":"3DS Universal-DB",
       "url":"https://db.universal-team.net/data/full.json",
       "type":"unistore",
       "enabled":true
    }]
  }
  ```
</details>

### Setting up remote repos (libget repos)
When `get` runs, it will go through the enabled repos in that config file, and try to make a GET request to `/repo.json`, which should contain (on the remote server) a listing of all of the packages and descriptions. Here is an example of what the remote's repo.json with one package looks like:
```
{
  "packages": [
    {
      "name": "space",
      "title": "Space Game",
      "author": "vgmoose",
      "description": "Shoot rocks in outer space, and stuff",
      "version": "1.0.0"
    }
  ]
}
```

The python script [repogen.py](https://github.com/vgmoose/get/blob/master/web/repogen.py) can generate the above repo.json file and the zip file structure explained below. It turns folders in the [packages](https://github.com/vgmoose/get/tree/master/web/packages) directory into zip files in `zips`.

### Installing a package
Installing a package requires the desired package name to exist in one of the repos, and for a GET to `/zips/$PKG_NAME.zip` to resolve. For example, to download the package `space` from above, the following command would be used:
```
./get space
```

It will try to fetch `/zips/space.zip` from the repo that contains the `space` package, and save it in `sd:/`.

### Listing all available packages
All available remote packages and their current status on disk will be listed.
```
./get -l
```

### Searching for a specific package
Search for a given string in the remote packages' name and description.
```
./get -s spa
```

### Removing an installed package
Any installed packages specified after the `--delete` flag will be removed
```
./get --delete space
```

This command parses the `manifest.install` file fetched when the package was installed, and uses it to determine which files to remove. Currently empty folders are left behind after the files are deleted.

## Building for PC
The following system-level dependencies are needed: zlib and libcurl. After obtaining those, it should be as straightforward as cloning the repo and running make:
```
git clone https://gitlab.com/4tu/libget.git
cd libget
make
```
The get CLI binary should now be sitting in: `./get`

This project also makes use of these libraries: [rapidjson](https://github.com/Tencent/rapidjson), [minizip](https://github.com/nmoinvaz/minizip/tree/1.2), and for some platforms [tinyxml](http://www.grinninglizard.com/tinyxml/). These libraries' sources are included in this repo in the `./src/libs` folder, and automatically included by the makefile.

**TODO**: point those libraries to their respective git repos via submodules.

## Including the library
The easiest way to include the project is to use Buck and Buckaroo in your project, and run:
```
buckaroo add github.com/vgmoose/libget
```

But you can also manually include it by downloading the repo and pointing to the root folder in your Makefile. See Makefile for information on how the library can be used from a GNU Makefile.

## License
This software is licensed under the GPLv3.

Contributors:
- rw-r-r_0644 - manifest file parsing code
- zarklord - zip folder extraction library

### Contributing
It's not required, but running a clang-format before making a PR helps to clean up styling issues:
```
find . \( -name "*.cpp" -or -name "*.hpp" \) -not -path "./src/libs/*" -exec clang-format -i {} \;
```
