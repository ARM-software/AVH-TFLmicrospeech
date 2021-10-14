
// Define I2C instance used to control audio codec device
#define WM8960_INSTANCE_I2C       1U

// Define I2C address used to address audio codec device
#define WM8960_ADDRESS_I2C        WM8960_I2C_ADDR

// Define codec device operation mode
#define WM8960_MODE_MASTER        false

// Define audio data transfer protocol
#define WM8960_BUS_PROTOCOL       kWM8960_BusI2S

// Define audio data route
#define WM8960_DATA_ROUTE         kWM8960_RoutePlaybackandRecord

// Define left input source
#define WM8960_INPUT_LEFT         kWM8960_InputDifferentialMicInput3

// Define right input source
#define WM8960_INPUT_RIGHT        kWM8960_InputDifferentialMicInput2

// Define input source for left and right mixer (i.e. output source to speaker)
#define WM8960_MIXER_SOURCE       kWM8960_PlaySourceDAC
