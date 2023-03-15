@echo off
"VHT_Corstone_SSE-300_Ethos-U55.exe" -V "..\VSI\audio\python" -f fvp_config.txt -a Objects\microspeech.axf --stat --simlimit 24 %*
