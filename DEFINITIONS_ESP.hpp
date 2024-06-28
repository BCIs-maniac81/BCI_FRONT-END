                                // ================================================== //
                                // ===============  DEFINITIONS_ESP.cpp ================ //
                                // ================================================== //
# ifndef ____DEFINITIONS_ESP____
# define ____DEFINITIONS_ESP____

# define spiClk 1000000 // ESP SCLK frequency in HZ.
# define BCI_LMSE_VERSION 1.0 // This is the first version of BCI.
# define MASK01 0b11110011 //(0xF3) Mask of Mode - initializing of (CPOL, CPHA) in SPCR.
# define MASK02 0b11111100 // (0xFC) Mask of CLOCK - initializing of (SPR1, SPR0) in SPCR.
# define MASK03 0b11111110 // (0xFE) Mask of Double-Clock - initializing of SPI2x in SPSR.
# define BIAS_SENS_MASK 0b11101110 //0xEE.
# define SPI_DATA_MODE 0b00000100 // (0x04) CPOL = 0 and CPHA = 1
# define SPI_CLOCK_DIV4 0b00000000 // (0x00) and SPI2x = 0
# define SPI_CLOCK_DIV2 0b00000000 // (0x00) and SPI2x = 1
# define SPI_CLOCK_DIV16 0b00000001 // (0x01) and SPI2x = 0
# define SPI_CLOCK_DIV8 0b00000001 // (0x01) and SPI2x = 1
# define SPI_CLOCK_DIV64 0b00000010 // (0x02) and SPI2x = 0
# define SPI_CLOCK_DIV32 0b00000010 // (0x02) and SPI2x = 1
# define SPI_CLOCK_DIV128 0b00000011 // (0x03) and SPI2x = 0
# define SPI_CLOCK_DIV64 0b00000011 // (0x03) and SPI2x = 1


// ADS-1299 Command Definitions with Byte Assignments (ADS1299 Datasheet, Page-40, 2017 Revision).
// System Commands

# define _WAKEUP  0b00000010 //(0x02) Wake-up from Standby Mode , and then must wait 4tCLK.
# define _STANDBY 0b00000100 //(0x04) Enter Low-Power StandBy Mode. 
# define _RESET 0b00000110 //(0x06) Reset the Device
# define _START 0b00001000 //(0x08) Start & Restart Conversions, Synchronize conversions in multiple Device
							// in a chained configuration.
# define _STOP 0b00001010 //(0x0A) Stop Conversion.
// Read Data Commands
# define _RDATAC 0b00010000 //(0x10) Enable Read Data Continuous Mode. default mode at Power-up.
# define _SDATAC 0b00010001 //(0x11) Stop Read Data Continuously Mode. After SDATAC wait 4tCLK before sending other commands.
# define _RDATA 0b00010010 //(0x12) Read Data by command. Supports multiple read back.
# define _BYTE1_RREG 0b00100000 //(0x20) this is just the first opcode to combinate with the starting register address.
# define _BYTE1_WREG 0b01000000 //(0x40) the first opcode to combinate with the starting register address.

// Addreses of the ADS1299 Internal Registrs according to the register Maps ---> Datasheet p.44
// *************  READ ONLY ID REGISTER ******************//
# define ID 0b00000000 //(0x00)

// *************  Global Settings Across Channels  ********* //
# define CONFIG1 0b00000001 //(0x01)
# define CONFIG2 0b00000010 //(0x02)
# define CONFIG3 0b00000011 //(0x03)
# define LOFF 0b00000100 //(0x04)

// ************  Channel-Specific Settings  **************** //
# define CH1SET 0b00000101 //(0x05)
# define CH2SET 0b00000110 //(0x06)
# define CH3SET 0b00000111 //(0x07)
# define CH4SET 0b00001000 //(0x08)
# define CH5SET 0b00001001 //(0x09)
# define CH6SET 0b00001010 //(0x0A)
# define CH7SET 0b00001011 //(0x0B)
# define CH8SET 0b00001100 //(0x0C)
# define BIAS_SENSP 0b00001101 //(0x0D)
# define BIAS_SENSN 0b00001110 //(0x0E)
# define LOFF_SENSP 0b00001111 //(0x0F)
# define LOFF_SENSN 0b00010000 //(0x10)
# define LOFF_FLIP 0b00010001 //(0x11)

// ************  Lead-Off Status Registers (Read-Only Registers)
# define LOFF_STATP 0b00010010 //(0x12)
# define LOFF_STATN 0b00010011 //(0x13)

// ************  GPIO and OTHER Registers ********************** //
# define GPIO_ADS 0b00010100 //(0x14)
# define MISC1 0b00010101 //(0x15)
# define MISC2 0b00010110 //(0x16)
# define CONFIG4 0b00010111 //(0x17)

// ************** GAIN SETTINGS CORRESPONDING TO CHnSET POSITIONS 6:4 *********//
# define PGA_GAIN1_SELECT (1) // --> GAIN = 1.
# define PGA_GAIN2_SELECT (2) // --> GAIN = 2.
# define PGA_GAIN4_SELECT (4) // --> GAIN = 4.
# define PGA_GAIN6_SELECT (6) // --> GAIN = 6.
# define PGA_GAIN8_SELECT (8) // --> GAIN = 8.
# define PGA_GAIN12_SELECT (12) // --> GAIN = 12.
# define PGA_GAIN24_SELECT (24) // --> GAIN = 24.

// ************* GAIN MASK SETTINGS ************ //
# define GAIN_SET_MASK  0b10001111  // (0x8F --> used to mask other bits )
# define PGA_GAIN1_MASK 0b00000000  //( 0x00 --> GAIN = 1)
# define PGA_GAIN2_MASK 0b00000001  //( 0x10 --> GAIN = 2)
# define PGA_GAIN4_MASK 0b00000010  //( 0x20 --> GAIN = 4)
# define PGA_GAIN6_MASK 0b00000011  //( 0x30 --> GAIN = 6)
# define PGA_GAIN8_MASK 0b00000100  //( 0x40 --> GAIN = 8)
# define PGA_GAIN12_MASK 0b00000101 //( 0x50 --> GAIN = 12)
# define PGA_GAIN24_MASK 0b00000110 //( 0x60 --> GAIN = 24)*/

//*************** CHANNEL INPUT SELECTION MASK CORRESPONDING TO CHnSET POSITIONS 2:0 *********//
# define INPUT_SELECT_MASK 0b11111000       // (0xF8)--> used to isolate the first three bits.
# define INPUT_NORMAL_MASK 0b00000000       //( 0x00 --> Input Normal)
# define INPUT_SHORTED_MASK 0b00000001      //( 0x01 --> Input Shorted)
# define INPUT_BIAS_MEAS_MASK 0b00000010    //( 0x02 --> Input for BIAS Measurements)
# define INPUT_MVDD_MEAS_MASK 0b00000011    //( 0x03 --> Input for MVDD Measurements)
# define INPUT_TEMP_SENS_MASK 0b00000100    //( 0x04 --> Temperature sensor)
# define INPUT_TEST_SIGNAL_MASK 0b00000101  // (0x05 --> Test Signal)
# define INPUT_POSITIVE_DRIVER_MASK 0b00000110 //(0x06 --> Positive electrode is the driver)
# define INPUT_NEGATIVE_DRIVER_MASK 0b00000111 //(0x07 --> Negative electrode is the driver)

//************************* OUTPUT DATA RATES *****************//
# define DATA_RATE_OPS_MASK 0b11111000 // (0xF8)--> used to isolate the first 3bits CONFIG1.
# define DATA_RATE_16K_MASK 0b00000000 // (0x00)--> fmod/64= 16KSPS.
# define DATA_RATE_08K_MASK 0b00000001 // (0x01)--> fmod/128= 8KSPS.
# define DATA_RATE_04K_MASK 0b00000010 // (0x02)--> fmod/256= 4KSPS.
# define DATA_RATE_02K_MASK 0b00000011 // (0x03)--> fmod/512= 2KSPS.
# define DATA_RATE_01K_MASK 0b00000100 // (0x04)--> fmod/1024= 1KSPS.
# define DATA_RATE_500_MASK 0b00000101 // (0x05)--> fmod/2048= 500SPS.
# define DATA_RATE_250_MASK 0b00000110 // (0x06)--> fmod/4096= 250SPS.

// *************************** DATA RATES INITIALIZATION VALUES ************************* //
// must be used in dataRateSettings(int _DR) function.
# define DR_250 250
# define DR_500 500
# define DR_1K 1000
# define DR_2K 2000
# define DR_4K 4000
# define DR_8K 8000
# define DR_16K 16000

// ********************************* TEST SIGNAL SOURCES ********************************** //
# define TEST_SIGNAL_SOURCE_MASK 0b11101111   //* (0xFC) used to isolate the 5th bit in CONFIG2.
# define TEST_SIGNAL_EXTERN_MASK 0b00000000  //* (0x00)--> test signals are generated externally. 
# define TEST_SIGNAL_INTERN_MASK 0b00010000  //* (0x10)--> test signals are generated internally.
// ***************************** TEST SIGNAL SOURCES SETTINGS *************************** //
// must be used in testSignalSources(uint8_t _test_source) function.
 # define TEST_SIG_EXTERNAL 0
 # define TEST_SIG_INTERNAL 1

// ************************* TEST SIGNAL SOURCE CODES ********************** //
//# define INTERNAL_TEST_SIGNAL_CODE 0b00010000 //  
//# define EXTERNAL_TEST_SIGNAL_CODE 0b00000000 // 

// *************************** TEST SIGNAL FREQUENCY ************************ //
# define TEST_SIGNAL_FREQ_MASK 	   0b11111100 //* (0xFC) used to isolate the first 2bits CONFIG2.
# define TEST_SIGNAL_FREQ21_MASK   0b00000000 //* (0x00)--> pulsed at fCLK/2^21.
# define TEST_SIGNAL_FREQ20_MASK   0b00000001 //* (0x01)--> pulsed at fCLK/2^20.
# define TEST_SIGNAl_DC_FREQ0_MASK 0b00000011 //* (0x11)--> At DC.

// ******************************* TEST SIGNAL FREQUENCY SETTINGS ********************** //
// must be used in testSignalFrequency(uint8_t _test_freq) function.
 # define TEST_SIG_FCLK_21 21   // freq f/2^21.
 # define TEST_SIG_FCLK_20 20   // freq f/2^22.
 # define TEST_SIG_FCLK_AT_DC 0 // type DC.

// *************************** TEST SIGNAL AMPLITUDE ************************ //
# define TEST_SIGNAL_AMP_MASK 	   0b11111011 //* (0xFB) used to isolate the 3rd bit in CONFIG2.
# define TEST_SIGNAL_SIMPLE_MASK   0b00000000 //* (0x00) calibration signal Amplitude = 1*-(VREFP - VREFN)/2400.
# define TEST_SIGNAL_DOUBLE_MASK   0b00000100 //* (0x04) calibration signal Amplitude = 2*-(VREFP - VREFN)/2400.

// ******************************* TEST SIGNAL AMPLITUDE SETTINGS ********************** //
// must be used in testSignalAmplitude(uint8_t _test_freq) function.
 # define TEST_SIG_AMP_1 1 // simple test signal.
 # define TEST_SIG_AMP_2 2 // double test signal.

// ********************************* POWER-DOWN REFERENCE BUFFER ********************************** //
# define PD_REFERENCE_MASK   0b01111111 // (0x7F) used to isolate the 7th bit in CONFIG3.
# define PD_REFERENCE_EXTERN_MASK   0b00000000 // (0x00)--> to enable the external reference buffer.
# define PD_REFERENCE_INTERN_MASK   0b10000000 // (0x80)--> to enable the internal reference buffer.

// ***************************** POWER-DOWN REFERENCE SETTINGS *************************** //
// must be used in pdInternalReferenceBuffer(uint8_t _pd_buffer) function.
 # define EXTERNAL_REF_BUFFER 0 // external reference buffer.
 # define INTERNAL_REF_BUFFER 1 // internal reference buffer.

// ********************************* BIAS REFERENCE SIGNAL ********************************** //
# define BIAS_RFERENCE_MASK   0b11110111 // (0xF7) used to isolate the 4th bit in CONFIG3.
# define BIAS_REFERENCE_EXTERN_MASK   0b00000000 // (0x00)--> to enable the external BIAS reference.
# define BIAS_REFERENCE_INTERN_MASK   0b00001000 // (0x08)--> to enable the internal BIAS reference = (AVDD + AVSS / 2).

// ***************************** BIAS REFERENCE SETTINGS *************************** //
// must be used in pdInternalReferenceBuffer(uint8_t _pd_buffer) function.
 # define EXTERNAL_BIAS_REF 0 // external BIAS reference.
 # define INTERNAL_BIAS_REF 1 // internal BIAS reference.

// ********************************* POWER-DOWN BIAS BUFFER ********************************** //
# define PD_BIAS_BUFFER_MASK      0b11111011 // (0xFB)--> to isolate the third bit in CONFIG3.
# define PD_BIAS_BUFFER_OFF_MASK  0b00000000 // (0x00)--> to power down the BIAS buffer.
# define PD_BIAS_BUFFER_ON_MASK   0b00000100 // (0x08)--> to enable the BIAS buffer.

// ***************************** POWER-DOWN BIAS BUFFER SETTINGS *************************** //
// must be used in biasBufferPower(uint8_t pd_BIAS_buffer) function.
 # define BIAS_BUFFER_OFF 0 // BIAS buffer is OFF.
 # define BIAS_BUFFER_ON 1  // BIAS buffer is ON.

// ************* SRB2 SETTING MASK CORRESPONDING TO CHnSET POSITION 3 ***** //
# define SRB2_CH_MASK 0b00001000 // (0x08 --> SRB2 switch is closed) 

//************************** CHANNEL INPUT ON/OFF CORRESPONDING TO CHnSET POSITION 7;
# define INPUT_ON_MASK 0b00000000 //(0x00 --> Channel is ON)
# define INPUT_OFF_MASK 0b10000000 //(0x80 --> Channel is OFF)

//**********************               ******************************//
typedef enum    {WITHOUT,WITH}    BOOL;
enum {CH_PWR_MODE, CH_GAIN, CH_SRB2,CH_INPUT_SELECT, CH_BIAS_ROUTING_P, CH_BIAS_ROUTING_N, BIAS_MEAS, CH_SRB1};

						// ROW where define Power mode either in DOWN (1) or NORMAL (0) operation mode --> CHnSET
						// ROW where define PGA_GAIN settings --> CHnSET
						// ROW where define SRB2 state --> CHnSET
						// ROW where define the channel input Mode -->CHnSET
						// ROW where define CH_BIAS_ROUTING --> SENSP && SENSN
						// ROW where define SRB1 state --> MISC1

// ***************************** COMMUNICATION START & END CODES ******************************* //

# define HEADER_FRAME 0xBB // 
# define FOOTER_FRAME 0xEE // 

// ****************************** OTHER DATA FORMAT PLATEFORM ********************** //
# define min_modular_EEG_int16 32767
# define max_modular_EEG_int16 -32767


# endif