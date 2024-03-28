# IQDB: Image Query Database System

IQDB is a reverse image search system. It lets you search a database of images
to find images that are visually similar to a given image.

this version of IQDB is forked from [Danbooru](https://github.com/danbooru/danbooru),
which is a fork from the original IQDB used by https://iqdb.org

this fork uses some changes from the [e621](https://github.com/e621ng/iqdb) fork

this fork changes the IDs to be `uint64` instead of `uint32`, and adds a `GET /images:id` handle (which itself was copied from e621's IQDB), and fixes the Docker build (uses Ubuntu 22.04)

# Quickstart

```bash
# build the docker image
docker build . -t $IMAGE_NAME

# Run IQDB in Docker on port 5588. This will create a database file in the current directory called `iqdb.sqlite`.
docker run --rm -it -p 5588:5588 -v $PWD:/mnt $IMAGE_NAME http 0.0.0.0 5588 /mnt/iqdb.sqlite

# Test that IQDB is running
curl -v http://localhost:5588/status

# Add `test.jpg` to IQDB with ID 1234. You will need to generate a unique ID for every image you add.
curl -F file=@test.jpg http://localhost:5588/images/1234

# Find images visually similar to `test.jpg`.
curl -F file=@test.jpg http://localhost:5588/query
```

# Usage

IQDB is a simple HTTP server with a JSON API. It has commands for adding
images, removing images, and searching for similar images. Image hashes are
stored on disk in an SQLite database.

#### Adding images

To add an image to the database, POST a file to `/images/:id` where `:id` is an
ID number for the image. On Danbooru, the IDs used are post IDs, but they can
be any number to identify the image.

```bash
curl -F file=@test.jpg http://localhost:5588/images/1234
```

```json
{
  "hash": "iqdb_3fe4c6d513c538413fadbc7235383ab23f97674a40909b92f27ff97af97df980fcfdfd00fd71fd77fd7efdfffe00fe7dfe7ffe80fee7fefeff00ff71ff7aff7fff80ffe7fff1fff4fffa00020008001d009d02830285028803020381038304850701078208000801f97df9fffb7afcfdfd77fe00fe7dfe80fefaff00ff7aff7ffffaffff00030007000e000f0010002000830087008e008f009000a0010c010e018202810283028502860290030203810383058306000b83f67afafdfb7ffcf7fcfefcfffd7dfef3fefafeffff7afffa00030007000e001000200080008400870088008e0090010001030107010e018001810183020d02810282029003030483048d0507050e0680",
  "post_id":1234,
  "signature":{
    "avglf":[0.6492715250149176,0.05807835483220937,0.022854957762458],
    "sig":[[-3457,-1670,-1667,-1664,-771,-768,-655,-649,-642,-513,-512,-387,-385,-384,-281,-258,-256,-143,-134,-129,-128,-25,-15,-12,-6,2,8,29,157,643,645,648,770,897,899,1157,1793,1922,2048,2049],[-1667,-1537,-1158,-771,-649,-512,-387,-384,-262,-256,-134,-129,-6,-1,3,7,14,15,16,32,131,135,142,143,144,160,268,270,386,641,643,645,646,656,770,897,899,1411,1536,2947],[-2438,-1283,-1153,-777,-770,-769,-643,-269,-262,-257,-134,-6,3,7,14,16,32,128,132,135,136,142,144,256,259,263,270,384,385,387,525,641,642,656,771,1155,1165,1287,1294,1664]]
  }
}
```

The `signature` is the raw IQDB signature of the image. Two images are similar
if their signatures are similar. The `hash` is the signature encoded as a hex
string.

#### Removing images

To remove an image to the database, do `DELETE /images/:id` where `:id` is the
ID number of the image.

```bash
curl -X DELETE http://localhost:5588/images/1234
```

```json
{ "post_id": 1234 }
```

#### Searching for images

To search for an image, POST a `file` to `/query?limit=N`, where `N` is the
maximum number of results to return (default 10).

```bash
curl -F file=@test.jpg 'http://localhost:5588/query?limit=10'
```

```json
[
  {
    "hash":"iqdb_3fe4c6d513c538413fadbc7235383ab23f97674a40909b92f27ff97af97df980fcfdfd00fd71fd77fd7efdfffe00fe7dfe7ffe80fee7fefeff00ff71ff7aff7fff80ffe7fff1fff4fffa00020008001d009d02830285028803020381038304850701078208000801f97df9fffb7afcfdfd77fe00fe7dfe80fefaff00ff7aff7ffffaffff00030007000e000f0010002000830087008e008f009000a0010c010e018202810283028502860290030203810383058306000b83f67afafdfb7ffcf7fcfefcfffd7dfef3fefafeffff7afffa00030007000e001000200080008400870088008e0090010001030107010e018001810183020d02810282029003030483048d0507050e0680",
    "post_id":1234,
    "score":100,
    "signature":{
      "avglf":[0.6492715250149176,0.05807835483220937,0.022854957762458],
      "sig":[[-3457,-1670,-1667,-1664,-771,-768,-655,-649,-642,-513,-512,-387,-385,-384,-281,-258,-256,-143,-134,-129,-128,-25,-15,-12,-6,2,8,29,157,643,645,648,770,897,899,1157,1793,1922,2048,2049],[-1667,-1537,-1158,-771,-649,-512,-387,-384,-262,-256,-134,-129,-6,-1,3,7,14,15,16,32,131,135,142,143,144,160,268,270,386,641,643,645,646,656,770,897,899,1411,1536,2947],[-2438,-1283,-1153,-777,-770,-769,-643,-269,-262,-257,-134,-6,3,7,14,16,32,128,132,135,136,142,144,256,259,263,270,384,385,387,525,641,642,656,771,1155,1165,1287,1294,1664]]
    }
  }
]
```

The response will contain the top N most similar images. The `score` field is
the similarity rating, from 0 to 100. The `post_id` is the ID of the image,
chosen when you added the image.

You will have to determine a good cutoff score yourself. Generally, 90+ is a
strong match, 70+ is weak match (possibly a false positive), and <50 is no
match.

# Compiling

IQDB requires the following dependencies to build:

* A C++ compiler
* [CMake 3.19+](https://cmake.org/install/)
* [LibGD](https://libgd.github.io/)
* [SQLite](https://www.sqlite.org/download.html)
* [Python 3](https://www.python.org/downloads)
* [Git](https://git-scm.com/downloads)

Run `make` to compile the project. The binary will be at `./build/release/src/iqdb`.

Run `make debug` to compile in debug mode. The binary will be at `./build/debug/src/iqdb`.

You can also run `cmake --preset release` then `cmake --build --preset release
--verbose` to build the project. `make` is simply a wrapper for these commands.

You can run `make docker` to build the docker image.

or use `docker build .` to build the docker image

See the [Dockerfile](./Dockerfile) for an example of which packages to install on Ubuntu.

# History

this version of IQDB is a fork from [Danbooru's IQDB](https://github.com/danbooru/iqdb),
and incorporates some changes from [e621's IQDB](https://github.com/e621ng/iqdb).
Danbooru's IQDB version of IQDB is a fork of the original [IQDB](https://iqdb.org/code),
written by [piespy](mailto:piespy@gmail.com). IQDB is based on code from
[imgSeek](https://sourceforge.net/projects/imgseek/), written by Ricardo
Niederberger Cabral. The IQDB algorithm is based on the paper
[Fast Multiresolution Image Querying](https://grail.cs.washington.edu/projects/query/)
by Charles E. Jacobs, Adam Finkelstein, and David H. Salesin.

IQDB is distributed under the terms of the GNU General Public License. See
[COPYING](./COPYING) for details.

# Further reading

* https://grail.cs.washington.edu/projects/query
* https://grail.cs.washington.edu/projects/query/mrquery.pdf
* https://cliutils.gitlab.io/modern-cmake/
* https://riptutorial.com/cmake
* https://github.com/yhirose/cpp-httplib
* https://hub.docker.com/repository/docker/evazion/iqdb
* https://github.com/e621ng/iqdb
