#!/bin/bash

if [[ $(git diff development.. --name-status -- src/ | python3 tools/extract_modified.py) ]];
then
    echo `git diff development.. --name-status -- src/ | python3 tools/extract_modified.py`
    git diff development.. -- `git diff development.. --name-status -- src/ | python3 tools/extract_modified.py` > test.diff
else
    echo "No new diff"
    rm -f test.diff
fi
