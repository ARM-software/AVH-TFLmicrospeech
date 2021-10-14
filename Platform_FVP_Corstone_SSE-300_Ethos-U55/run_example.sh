#!/usr/bin/env bash
VHT-Corstone-300 -V "../VSI/audio/python" -f fvp_config.txt -a Objects/microspeech.axf --stat --cyclelimit 768000000 $*
