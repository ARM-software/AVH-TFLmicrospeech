
# ML Example

## Micro speech for TensorFlow Lite

Micro speech example for [TensorFlow Lite for Microcontrollers](https://www.tensorflow.org/lite/microcontrollers).

### Prerequisites
 - toolchain (IDE):
   - [MDK Microcontroller Development Kit](https://www.keil.com/mdk5)
 - toolchain (alternative: command-line tools):
   - [CMSIS-Build](https://github.com/ARM-software/CMSIS_5/releases/download/5.7.0/cbuild_install.0.10.2.sh) that is a component of the [CMSIS5 release](https://github.com/ARM-software/CMSIS_5/releases)
 - Fast Models:
   - FVP model for Corstone-300 MPS3 with VSI (example for FVP with user Python script)
   - [FVP model for Corstone-300 MPS3](https://developer.arm.com/tools-and-software/open-source-software/arm-platforms-software/arm-ecosystem-fvps)
     (example for FVP)
 - [Python 3](https://www.python.org/downloads/) (example for FVP with user Python script)

## Micro speech for FVP (Fixed Virtual Platform) for Corstone SSE-300 with Ethos-U55

Project directory: `./Platform_FVP_Corstone_SSE-300_Ethos-U55/`

Example has the following targets:
 - `Example`: FVP with Python interface (VSI) with audio test data provided through user Python script
    - Runs on the special Corstone SSE-300 Ethos-U55 FVP with Python interface providing Virtual Streaming Interface (VSI).<br/>
      It is required to install the model executable and binaries in order to run this example.<br/>
      Expected installation directory on Windows: `C:\Program Files\Arm\VHT\models\Win64_VC2019`<br/> 
      If installed in a different directory then this needs to be reflected:
       - in uVision project (Options for Target - Debug - Settings for Models Armv8-M Debugger) when running with MDK or
       - in script `run_example.cmd` when running standalone
    - Audio test data is provided by Python script `./VSI/audio/python/arm_vsi0.py` from WAVE file `test.wav` which contains keywords 'Yes' and 'No' alternating three times.
    - Build example with MDK using uVision project `microspeech.uvprojx` target `Example` or CMSIS-Build using `microspeech.Example.cprj` project.
    - Run example with MDK or standalone with script `run_example.cmd`.
    - When running the example the audio data is processed and any detected keywords are output to the terminal:
      ```
      Heard yes (152) @1100ms
      Heard no (141) @5500ms
      Heard yes (147) @9100ms
      Heard no (148) @13600ms
      Heard yes (147) @17100ms
      Heard no (148) @21600ms
      ```
 - `Example Test`: internal test for Example
 - `Audio Provider Mock`: public FVP with audio test data embedded in code
    - Runs on the public Corstone SSE-300 Ethos-U55 FVP.
    - Audio test data is embedded in the test code and contains keywords 'Yes' and 'No' alternating indefinitely.
    - Build example with MDK using uVision project `microspeech.uvprojx` target `Audio Provider Mock` or CMSIS-Build using `microspeech.Audio_Provider_Mock.cprj` project.
    - Run example with MDK or standalone.
    - When running the example the audio data is processed and any detected keywords are output to the terminal:
      ```
      Heard silence (149) @400ms
      Heard yes (158) @1200ms
      Heard no (143) @5600ms
      Heard yes (149) @9100ms
      Heard no (142) @13600ms
      Heard yes (149) @17100ms
      Heard no (142) @21600ms
      ```
 - `Audio Provider Mock Test`: internal test for Audio Provider Mock

### Micro speech for MIMXRT1064-EVK board

Project directory: `./Platform_MIMXRT1064-EVK/`

This example shows how to run a voice recognition model that can recognize 2 keywords, "yes" and "no",
from boards built-in microphone. Recognized keywords are written to the terminal.

 - Build example with MDK using uVision project `microspeech.uvprojx` or CMSIS-Build using `microspeech.MIMXRT1064-EVK.cprj` project.
 - Program and run the example with MDK or use Drag-and-drop programming through the DAP-Link drive.
 - Open the DAP-Link Virtual COM port in a terminal (baudrate = 115200) and monitor recognized keywords.

### Micro speech for IMXRT1050-EVKB board

Project directory: `./Platform_IMXRT1050-EVKB/`

This example shows how to run a voice recognition model that can recognize 2 keywords, "yes" and "no",
from boards built-in microphone. Recognized keywords are written to the terminal.

 - Build example with MDK using uVision project `microspeech.uvprojx` or CMSIS-Build using `microspeech.IMXRT1050-EVKB.cprj` project.
 - Program and run the example with MDK or use Drag-and-drop programming through the DAP-Link drive.
 - Open the DAP-Link Virtual COM port in a terminal (baudrate = 115200) and monitor recognized keywords.
