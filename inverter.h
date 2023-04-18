#include <Arduino.h>

//Send and receive periodic inverter commands
void serviceInverter();
bool sendCommand(String com);


enum qCommand
{
  QPI,
  QID,
  QVFW,
  QVFW2,
  QPIRI,
  QFLAG,
  QPIGS,
  QPIGSn,
  QMOD,
  QPIWS,
  QDI,
  QBOOT,
  QOPM,
  QT,
  DAT,
 };

void requestInverter(qCommand);

struct QpiMessage
{
  byte protocolId;
};

struct QpigsMessage
{
  float gridV;
  float gridHz;
  float acOutV;
  float acOutHz;
  short acOutVa;
  short acOutW;
  short acOutPercent;
  float battV;  
  float battV_SCC;
  float battV_SCC_2;
  byte battDischargeA;
  byte battChargeA;
  short battPercent;
  short heatSinkDegC;
  short MPPT1_charger_temp;
  short MPPT2_charger_temp;
  float solarW_PV1;
  float solarW_PV2;
  float solarV_PV1;
  float solarV_PV2;
  short ConfigurationState;
  short MPPT1_Status;
  short MPPT2_Status;
  short LoadConn;
  short battDirection;
  short DC_AC_Direction;
  short LineDirection;
  short Parallel_ID;
};

struct QpigsnMessage
{
  byte Parallel_ID;
  byte workMode;
  byte faultCode;
  float gridV;
  float gridHz;
  float acOutV;
  float acOutHz;
  short acOutVa;
  short acOutW;
  float totalAcOutVa;
  float totalAcOutW;
  float acOutPercent;
  float totAcOutPercent;
  float battV;  
  byte battDischargeA;
  byte battChargeA;
  short totBatChargeA;
  short batCapacity;
  float solarW_PV1;
  float solarW_PV2;
  float solarV_PV1;
  float solarV_PV2;
  short MPPT1_Status;
  short MPPT2_Status;
  short LoadConn;
  short battDirection;
  short DC_AC_Direction;
  short LineDirection;
  short maxTemp;
};

struct QmodMessage
{
  char mode;
  String operationMode;
  byte operationMode_RAW;
};

struct QpiwsMessage
{
  bool faultCode;
  bool lineFail;
  bool outCircShort;
  bool overTemperature;
  bool fanLocked;
  bool batteryVoltageHigh;
  bool inverterVoltLow;
  bool batteryUnderShutdown;
  bool overload;
  bool eepromFault;
  bool powerLimit;
  bool pvVoltageHigh;
  bool pv2VoltageHigh;
  bool mpptOverloadFault;
  bool mppt2OverloadFault;
  bool batteryTooLow4SCC1;
  bool batteryTooLow4SCC2;
};

struct QflagMessage
{
  bool disableBuzzer;
  bool enableOverloadBypass;
  bool enablePowerSaving;
  bool enableLcdEscape;
  bool enableOverloadRestart;
  bool enableOvertempRestart;
  bool enableBacklight;
  bool enablePrimarySourceInterruptedAlarm;
  bool enableFaultCodeRecording;
};

//QPIRI<cr>: Device Rating Information inquiry
struct QpiriMessage
{
  float gridRatingV;   //Grid rating voltage
  float gridRatingA;   //Grid rating current
  float acOutRatingV;  //AC output rating voltage
  float acOutRatingHz; //AC output rating frequency
  float acOutRatingA;  //AC output rating current
  float acOutRatungVA; //AC output rating apparent power
  float acOutRatingW;  //AC output rating active power
  float battRatingV;   //Battery rating voltage
  float battreChargeV; //Battery re-charge voltage
  float battDischargeV; //Battery re-discharge voltage
  float battUnderV;    //Battery under voltage
  float battBulkV;     //Battery bulk voltage
  float battFloatV;    //Battery float voltage
  String battType;     //Battery type
  byte battMaxAcChrgA; //Current max AC charging current
  byte battMaxChrgA;   //Current max charging current
  byte imputVrange;  //Input voltage range
  byte outSourcePrior; //Output source priority
  byte chargerSourcePrior; //Charger source priority
  byte parallelMax; //Parallel max num
  byte MachineType; //Machine type
  byte topology; // Topology
  byte outModelSett; //Output model setting
  byte SolarPowPrior; //Solar power priority
  byte mpptString; //MPPT string
};
//for future use
struct QmdMessage
{
};

struct QidMessage
{
  char id[16];
};
struct QtMessage
{
  char qt[16];
};

struct QdatMessage
{
  byte dt;
  //String currentDate;
};


//for raw answer from inverter
struct QRaw
{
  String QPIGS;
  String QPIGSn;
  String QPIRI;
  String QMOD;
  String QPIWS;
  String QFLAG;
  String QID;
  String QPI;
  String QET;
  String QT;
  String DAT;
  //String SETTIME;
};
