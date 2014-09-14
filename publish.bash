#!/bin/bash
#
nikola build
rsync -avz output/ paphus@paphus.com:~/fizz.buzz/
