#!/usr/bin/env bash
VHT_Corstone_SSE-300_Ethos-U55 -V "../VSI/audio/python" -f fvp_config.txt -a Objects/microspeech.axf --stat --cyclelimit 2400000000 $*
