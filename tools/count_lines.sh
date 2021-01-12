#!/bin/bash

find src/ resources/ -name '*.cpp' -or -name "*.c" -or -name "*.h" -or -name "*.frag" -or -name "*.vert" -or -name "*.py" -or -name "*.node" | xargs wc -l
