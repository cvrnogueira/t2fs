#!/bin/bash

echo "clear all compiled files"
make clean

echo "compile t2fs library"
make

echo "compile dev test"
make dev

echo "execute dev test"
./dev