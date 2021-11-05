@echo off
"C:\Program Files\ARM\VHT\models\Win64_VC2019\VHT-Corstone-300.exe" -V "..\VSI\audio\python" -f fvp_config.txt -a Objects\microspeech.axf --stat --cyclelimit 2400000000 %*
