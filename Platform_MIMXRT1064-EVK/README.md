Micro Speech Example
====================

This example shows how to run a voice recognition model that can recognize 2 keywords, "yes" and "no",
from boards built-in microphone. Recognized keywords are written to the terminal.

RTOS: Keil RTX5 Real-Time Operating System
------------------------------------------

The real-time operating system [Keil RTX5](https://arm-software.github.io/CMSIS_5/RTOS2/html/rtx5_impl.html) implements the resource management. 

It is configured with the following settings:

- [Global Dynamic Memory size](https://arm-software.github.io/CMSIS_5/RTOS2/html/config_rtx5.html#systemConfig): 24000 bytes
- [Default Thread Stack size](https://arm-software.github.io/CMSIS_5/RTOS2/html/config_rtx5.html#threadConfig): 3072 bytes
- [Event Recorder Configuration](https://arm-software.github.io/CMSIS_5/RTOS2/html/config_rtx5.html#evtrecConfig)
  - [Global Initialization](https://arm-software.github.io/CMSIS_5/RTOS2/html/config_rtx5.html#evtrecConfigGlobIni): 1
    - Start Recording: 1

Refer to [Configure RTX v5](https://arm-software.github.io/CMSIS_5/RTOS2/html/config_rtx5.html) for a detailed description of all configuration options.

Board: NXP MIMXRT1064-EVK
-------------------------

The tables below list the device configuration for this board. The board layer for the NXP MIMXRT1064-EVK is using the software component `::Board Support: SDK Project Template: project_template (Variant: evkmimxrt1064)` from `NXP.EVK-MIMXRT1064_BSP.12.3.0` pack.

The heap/stack setup and the CMSIS-Driver assignment is in configuration files of related software components.

The example project can be re-configured to work on custom hardware. Refer to ["Configuring Example Projects with MCUXpresso Config Tools"](https://github.com/MDK-Packs/Documentation/tree/master/Using_MCUXpresso) for information.

### System Configuration

| System Component        | Setting
|:------------------------|:----------------------------------------
| Device                  | MIMXRT1064DVL6A
| Board                   | MIMXRT1064-EVK
| SDK Version             | ksdk2_0
| Heap                    | 64 kB (configured in linker script MIMXRT1064xxxxx_*.scf file)
| Stack (MSP)             |  1 kB (configured in linker script MIMXRT1064xxxxx_*.scf file)

### Clock Configuration

| Clock                   | Setting
|:------------------------|:----------------------------------------
| AHB_CLK_ROOT            | 600 MHz
| IPG_CLK_ROOT            | 150 MHz
| PERCLK_CLK_ROOT         |  75 MHz
| USDHC1_CLK_ROOT         | 198 MHz
| UART_CLK_ROOT           |  80 MHz
| ENET_125M_CLK           |  50 MHz
| ENET_25M_REF_CLK        |  25 MHz
| ENET_TX_CLK             |  50 MHz

**Note:** configured with Functional Group: `BOARD_BootClockRUN`

### GPIO Configuration and usage

| Functional Group       | Pin | Peripheral | Signal      | Identifier  | Pin Settings                                        | Usage
|:-----------------------|:----|:-----------|:------------|:------------|:----------------------------------------------------|:-----
| BOARD_InitDEBUG_UART   | K14 | LPUART1    | TX          | UART1_TXD   | default                                             | UART1 TX for debug console (GPIO_AD_B0_12)
| BOARD_InitDEBUG_UART   | L14 | LPUART1    | RX          | UART1_RXD   | default                                             | UART1 RX for debug console (GPIO_AD_B0_13)
| BOARD_InitENET         | A7  | ENET       | MDC         | ENET_MDC    | default                                             | Ethernet KSZ8081RNB pin MDC (GPIO_EMC_40)
| BOARD_InitENET         | C7  | ENET       | MDIO        | ENET_MDIO   | default                                             | Ethernet KSZ8081RNB pin MDIO (GPIO_EMC_41)
| BOARD_InitENET         | B13 | ENET       | REF_CLK     | ENET_TX_CLK | default                                             | Ethernet KSZ8081RNB pin XI (GPIO_B1_10)
| BOARD_InitENET         | E12 | ENET       | RX_DATA, 0  | ENET_RXD0   | default                                             | Ethernet KSZ8081RNB pin RXD0 (GPIO_B1_04)
| BOARD_InitENET         | D12 | ENET       | RX_DATA, 1  | ENET_RXD1   | default                                             | Ethernet KSZ8081RNB pin RXD1 (GPIO_B1_05)
| BOARD_InitENET         | C12 | ENET       | RX_EN       | ENET_CRS_DV | default                                             | Ethernet KSZ8081RNB pin CRS_DV (GPIO_B1_06)
| BOARD_InitENET         | C13 | ENET       | RX_ER       | ENET_RXER   | default                                             | Ethernet KSZ8081RNB pin RXER (GPIO_B1_11)
| BOARD_InitENET         | B12 | ENET       | TX_DATA, 0  | ENET_TXD0   | default                                             | Ethernet KSZ8081RNB pin TXD0 (GPIO_B1_07)
| BOARD_InitENET         | A12 | ENET       | TX_DATA, 1  | ENET_TXD1   | default                                             | Ethernet KSZ8081RNB pin TXD1 (GPIO_B1_08)
| BOARD_InitENET         | A13 | ENET       | TX_EN       | ENET_TXEN   | default                                             | Ethernet KSZ8081RNB pin TXEN (GPIO_B1_09)
| BOARD_InitUSDHC        | J2  | USDHC1     | DATA, 3     | SD1_D3      | default                                             | SD Card pin D3 (GPIO_SD_B0_05)
| BOARD_InitUSDHC        | H2  | USDHC1     | DATA, 2     | SD1_D2      | default                                             | SD Card pin D2 (GPIO_SD_B0_04)
| BOARD_InitUSDHC        | K1  | USDHC1     | DATA, 1     | SD1_D1      | default                                             | SD Card pin D1 (GPIO_SD_B0_03)
| BOARD_InitUSDHC        | J1  | USDHC1     | DATA, 0     | SD1_D0      | default                                             | SD Card pin D0 (GPIO_SD_B0_02)
| BOARD_InitUSDHC        | J4  | USDHC1     | CMD         | SD1_CMD     | default                                             | SD Card pin CMD (GPIO_SD_B0_00)
| BOARD_InitUSDHC        | J3  | USDHC1     | CLK         | SD1_CLK     | default                                             | SD Card pin CLK (GPIO_SD_B0_01)
| BOARD_InitARDUINO_UART | J12 | LPUART3    | TX          | LPUART3_TX  | default                                             | Arduino UNO R3 pin D1 (GPIO_AD_B1_06)
| BOARD_InitARDUINO_UART | K10 | LPUART3    | RX          | LPUART3_RX  | default                                             | Arduino UNO R3 pin D0 (GPIO_AD_B1_07)
| BOARD_InitUSER_LED     | F14 | GPIO1      | gpio_io, 09 | USER_LED    | Direction Output, GPIO initial state 1, mode PullUp | User LED (GPIO_AD_B0_09)
| BOARD_InitUSER_BUTTON  | L6  | GPIO5      | gpio_io, 00 | USER_BUTTON | Direction Input, mode PullUp                        | User Button SW8 (WAKEUP)

### NVIC Configuration

| NVIC Interrupt      | Priority
|:--------------------|:--------
| ENET                | 8
| USDHC1              | 8
| LPUART3             | 8

**STDIO** is routed to debug console through Virtual COM port (DAP-Link, peripheral = LPUART1, baudrate = 115200)

### CMSIS-Driver mapping

| CMSIS-Driver | Peripheral
|:-------------|:----------
| ETH_MAC0     | ENET
| ETH_PHY0     | KSZ8081RNB (external)
| MCI0         | USDHC1
| USART3       | LPUART3

| CMSIS-Driver VIO  | Physical board hardware
|:------------------|:-----------------------
| vioBUTTON0        | User Button SW8 (WAKEUP)
| vioLED0           | User LED (GPIO_AD_B0_09)
