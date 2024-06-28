                                // ================================================== //
                                // ===============  BCI_LMSE_ESP.hpp ================ //
                                // ================================================== //
# ifndef ____BCI_LMSE_ESP____
# define ____BCI_LMSE_ESP____

# include <DEFINITIONS_ESP.hpp>
# include <stdio.h>
# include <stdint.h>
# include <Arduino.h>



//**** breakout ESP32 ****//
/* DRDY = 12, RESET = 13, CS = 15, MOSI = 23, MISO = 19, SCK = 18 */

class BCI_LMSE_ESP {
public:

    
    int DRDY, CS, RST;       //* pin numbers for "Data Ready" (DRDY), "Chip Select" CS (Datasheet, pg. 26) and /Reset.

    int DIV_01, DIV_02;      //
    boolean readOnce = true; //* will be used to read register only once, needed for execution time reduction.
    boolean writeOnce = true;//* will be used to read register only once, needed for execution time reduction.
    byte status_reg[3];      //* it saves the temporary stats. 
    byte mirror[24];         //* array used to mirror register Data. (24 * 8bits), while ADS1299 has 24 internal registers.
    BOOL details;            //* set true if need to show some details in the terminal, if not set to false. 
    int number_CH = 8;       // assume 8 channel (without Daisy chain)
    uint8_t counter = 0;
    boolean _32bit_2comp_format = true;

    byte bufferByte=0x00;
    long int channel[8];     //* array to record 8 channels streaming values at each moment.
    byte channelDataRaw[24]; // to save the raw data.
    byte _pop_read =0x00;   // pop operation to read output buffered data.
    
    byte CHnSET0 = 0x00;    //* First Buffer used to read internal CHnSET registers values.
    byte BIAS_P = 0x00;     //* buffer for postive BIAS_SENS.
    byte BIAS_N = 0x00;     //* buffer for Negative BIAS_SENS
    byte MISC1_buff = 0x00; //* buffer for MISC1 register.
    byte CONFIGn = 0x00;    //* buffer for CONFIG1, CONFIG2, CONFIG3 and CONFIG4.

    char leadOffSettings[8][2]; //* settings for channel lead-off states. 
    int channelSettings[8][8];  //* array to hold current channel settings.
    uint8_t pga_gain_settings = 0; //* used inside powerOnChannels method to set a MASK into CHnSET settings.

    //**** write Data To serial port --> binary format

    long int buff1; // 1st buffer 
    byte* (buff2) = (byte*) (&buff1);
    //boolean usingsimulatedData = false;

    //**** write Data To serial port --> ModularEEG format.
    byte sync0;
    byte sync1;
    byte version;
    byte foo;

    long buff_val32;
    int buff_val_i16;
    unsigned int buff_val1_u16;
    byte*  buff_val2_u16 = (byte *)(&buff_val1_u16);

    byte switches;
   
    boolean BIAS_SENS_routing[8];      //* Register Setter for channelSettings array.
    boolean SRB1_include = false;      // setter value for channelSettings array.
    boolean SRB2_include[8] = {false, false, false, false, false, false, false, false};  
                        //* Register Setter for channelSetting array.
    boolean daisy_chain_mode = true; 		    // daisy_chain mode if true.
    boolean multiple_readback_mode = true;      //	Multiple readback mode if daisy_chain mode is false.
    boolean signal_bias_in_meas = true;		    // used to measure bias signal if true.
    boolean bias_loff_sens = true;		        // BIAS sens function is disabled if false or enabled if true.
    boolean isADSWorking;
    boolean positive_input_is_ref = true;
    boolean internal_biasref_signal = true;

    boolean test = false;

     // ******   PROTOTYPES **********

    //***************** Initialization Sequences ************************** //
    void initialize(int _ESP_DRDY, int _ESP_CS, int _ESP_RST); // ESP_SPI settings;
    void resetADS1299();
    void daisyChainModeTest();		// to determine if device in Daisy-Chain mode or in multiple readBack mode.
    void dataRateSettings(int _DR); // to set the Data Rate. --> DR[2:0] CONFIG1. 
    void testSignalSources(int _test_source);  // to set the test signals source --> bit4 CONFIG2.
    void testSignalAmplitude(int _test_ampli); // to set the test signals amplitude --> bit2 CONFIG2.
    void testSignalFrequency(int _test_freq);  // to determine the calibration signal frequency.
    void performTestSignal(uint8_t numCH, int gain_selection, int _signal_source, int _signal_amp, int _signal_freq); 
                                            // This function is used to perform any signal test.
    void pdInternalReferenceBuffer(int _pd_ref_buffer); // to control the internal reference buffer -->bit7 CONFIG3 
    void biasSignalMeasurements(uint8_t numCH); // to enable BIAS measurements with any channel.
    void biasSignalReference(int _bias_ref);    // to set the BIASREF signal, default is internally.
    void biasBufferPower(int pd_BIAS_buffer); 	// to determine the BIAS buffer power state, default is ON.
    void biasSensFunction();	// to enable the BIAS sens function. 
    void biasLeadOffState();	// to read-only the BIAS status (connected or not). 
    void temperatureMeasurement(uint8_t numCH, int gain_selection);
    void mirrorInitialize(); 	// RESET MIRROR
    void setMirrorToDefault();  // set all register values following the datashet.
    void getMirrorContent();    // allow to view the content of te mirror array.

    // ************* SETTERS ******************** //
    void SRB2SETTER(uint8_t numCH); // to enable SRB2 on one channel or more.
    void SRB1SETTER();  // to enable SRB1 to all channels.
    void BIAS_SENS_ROUTING(uint8_t numCH);

    // ************* System Commands functions ********************** // // **** for more information see Datasheet page.40
    void WAKEUP();      // function to Wake-up from Standby low energy Mode.
    void STANDBY();     // Standby mode function.
    void RESET();       // Reset the Device function.
    void START();       // Start or Restart (synchronize) Conversion Function.
    void STOP();        // Stop conversion function.

    // ************* Data Read Commands  *************************** //
    void RDATAC();      // Read Data Continuous Mode Function.
    void SDATAC();      // Stop Read Data Continuously Function.
    void RDATA();       // Read Data by Command Function.
    void RDATA2();      // Send _RDATA command to enter in RDATA mode
    
    void startADSStreaming();// strat streaming data
    void stopADSStreaming(); // Stop streaming Data
    int isDataReady();      // to test if Data are ready (DRDY\).
    void updateChannels(boolean _32bit_2comp_format);   // 
    void allChannelsTxTPrint(long sample);
    void oneChannelTxTPrint(uint8_t numCH, long sample);
    void BinaryDataSenderToSerial(uint8_t bci_dimension,long sample);
    void dataSenderToSerialAsOpenEEG_brainBay(long signal_samples);

    // ************ Register Read Commands ***************************//
    byte ADS1299ID();
    void ADS1299ID2();
    byte RREG(byte _regAdd);
    void RREGS(byte _regAdd, byte _second_command); // To read Multiple Consecutive Registers (DATASHEET page-43)
    void registerNameDisplay(byte _regAdd);
    void WREG(byte _regAdd, byte _value); //
    void WREGS(byte _regAdd, byte _second_command); //
    void DecToHex(byte _regData); // function that allow to view returned data in Hexadecimal format.
    
    //************************* Data Channel Control ************************* ///
    void zeroChannelSettings();
    void powerDownChannels(uint8_t numCH, boolean openSRB2);
    void powerOnChannels(uint8_t numCH, uint8_t input_selection, int gain_selection);
    boolean testChannelActivity(uint8_t numbCH);
    void defaultChannelSettings(uint8_t numbCH);
    void channelSettingsStats(void);
    void resetStatusRegister();
    void resetChannelsData();


    ///************************** SPI transfer command *********************************//
    
    //SPI Arduino Library Stuff
    byte SPI_transfer(byte _data);

    //------------------------//
    // void attachInterrupt();
    // void detachInterrupt(); // Default
    // void begin(); // Default
    // void end();
    // void setBitOrder(uint8_t);
    // void setDataMode(uint8_t);
    // void setClockDivider(uint8_t);
    //------------------------//
    
 
    
    };

# endif
