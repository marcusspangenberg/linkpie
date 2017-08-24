#!/bin/bash

git clone https://github.com/Ableton/link.git
pushd link
git submodule update --init --recursive
popd
