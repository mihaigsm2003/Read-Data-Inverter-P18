#include "Arduino.h"
#include "inverter.h"

#include "settings.h"

extern Settings _settings;
extern String currentDate;

String _commandBuffer;

QpiMessage _qpiMessage = {0};
QpigsMessage _qpigsMessage = {0};
QpigsnMessage _qpigsnMessage = {0};
QmodMessage _qmodMessage = {0};
QpiwsMessage _qpiwsMessage = {0};
QflagMessage _qflagMessage = {0};
QidMessage _qidMessage = {0};
//QchgcrMessage _qchgcrMessage ={0};
QpiriMessage _qpiriMessage = {0};
QtMessage _qtMessage = {0};
QdatMessage _qdatMessage = {0};

QRaw _qRaw;
//QAv _qAv;

//Found here: http://forums.aeva.asn.au/pip4048ms-inverter_topic4332_post53760.html#53760
#define INT16U unsigned int
#define INT8U byte

#define SERIALDEBUG

//short map function for float mapping
float mapf(float value, float fromLow, float fromHigh, float toLow, float toHigh)
{
  float result;
  result = (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
  return result;
}

unsigned short cal_crc_half(byte *pin, byte len)
{
  unsigned short crc;
  byte da;
  byte *ptr;
  byte bCRCHign;
  byte bCRCLow;
  
  const unsigned short crc_ta[16] =
      {
          0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
          0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef};

  ptr = pin;
  crc = 0;
  while (len-- != 0)
  {
    da = ((byte)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*ptr >> 4)];
    da = ((byte)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*ptr & 0x0f)];
    ptr++;
  }
  bCRCLow = crc;
  bCRCHign = (byte)(crc >> 8);
  if (bCRCLow == 0x28 || bCRCLow == 0x0d || bCRCLow == 0x0a)
  {
    bCRCLow++;
  }
  if (bCRCHign == 0x28 || bCRCHign == 0x0d || bCRCHign == 0x0a)
  {
    bCRCHign++;
  }
  crc = ((unsigned short)bCRCHign) << 8;
  crc += bCRCLow;
  return (crc);
}

//Parses out the next number in the command string, starting at index
//updates index as it goes
float getNextFloat(String &command, int &index)
{
  String term = "";
  while (index < (int)command.length())
  {
    char c = command[index];
    ++index;

    if ((c == '.') || (c == '+') || (c == '-') || ((c >= '0') && (c <= '9')))
    {
      term += c;
    }
    else
    {
      return term.toFloat();
    }
  }
  return 0;
}

//Parses out the next number in the command string, starting at index
//updates index as it goes
long getNextLong(String &command, int &index)
{
  String term = "";
  while (index < (int)command.length())
  {
    char c = command[index];
    ++index;

    if ((c == '.') || ((c >= '0') && (c <= '9')))
    {
      term += c;
    }
    else
    {
      return term.toInt();
    }
  }
  return 0;
}

//Gets if the next character is '1'
bool getNextBit(String &command, int &index)
{
  String term = "";
  if (index < (int)command.length())
  {
    char c = command[index];
    ++index;
    return c == '1';
  }
  return false;
}

bool onPIGS() //QPIGS<cr>: Device general status parameters inquiry
{
  //if(_commandBuffer[1]=='N') _qAv.QPIGS = false; //deactivate if NAK
  if (_commandBuffer.length() < 60)
  {
    return false;
  }
  else
  {
    _qRaw.QPIGS = _commandBuffer; //^P005GS  General Status
    int index = 5; //after the starting '^D106'
      _qpigsMessage.gridV = (short)getNextLong(_commandBuffer, index)*0.1;               //1
      _qpigsMessage.gridHz = (short)getNextLong(_commandBuffer, index)*0.1;              //2
      _qpigsMessage.acOutV = (short)getNextLong(_commandBuffer, index)*0.1;              //3
      _qpigsMessage.acOutHz = (short)getNextLong(_commandBuffer, index)*0.1;             //4
      _qpigsMessage.acOutVa = (short)getNextLong(_commandBuffer, index);             //5
      _qpigsMessage.acOutW = (short)getNextLong(_commandBuffer, index);              //6
      _qpigsMessage.acOutPercent = (byte)getNextLong(_commandBuffer, index);         //7
      _qpigsMessage.battV = (short)getNextLong(_commandBuffer, index)*0.1;               //8
      _qpigsMessage.battV_SCC = (short)getNextLong(_commandBuffer, index)*0.1;           //9    
      _qpigsMessage.battV_SCC_2 = (short)getNextLong(_commandBuffer, index)*0.1;         //10
      _qpigsMessage.battDischargeA = (short)getNextLong(_commandBuffer, index);      //11
      _qpigsMessage.battChargeA = (short)getNextLong(_commandBuffer, index);         //12
      _qpigsMessage.battPercent = (short)getNextLong(_commandBuffer, index);         //13
      _qpigsMessage.heatSinkDegC = (short)getNextLong(_commandBuffer, index);        //14
      _qpigsMessage.MPPT1_charger_temp = (short)getNextLong(_commandBuffer, index);  //15
      _qpigsMessage.MPPT2_charger_temp = (short)getNextLong(_commandBuffer, index);  //16      
      _qpigsMessage.solarW_PV1 = (short)getNextLong(_commandBuffer, index);          //17
      _qpigsMessage.solarW_PV2 = (short)getNextLong(_commandBuffer, index);          //18
      _qpigsMessage.solarV_PV1 = (short)getNextLong(_commandBuffer, index)*0.1;          //19
      _qpigsMessage.solarV_PV2 = (short)getNextLong(_commandBuffer, index)*0.1;          //20
      _qpigsMessage.ConfigurationState = (byte)getNextLong(_commandBuffer, index);   //21
      _qpigsMessage.MPPT1_Status = (byte)getNextLong(_commandBuffer, index);         //22
      _qpigsMessage.MPPT2_Status = (byte)getNextLong(_commandBuffer, index);         //23
      _qpigsMessage.LoadConn = (byte)getNextLong(_commandBuffer, index);             //24
      _qpigsMessage.battDirection = (byte)getNextLong(_commandBuffer, index);        //25
      _qpigsMessage.DC_AC_Direction = (byte)getNextLong(_commandBuffer, index);      //26
      _qpigsMessage.LineDirection = (byte)getNextLong(_commandBuffer, index);        //27
      _qpigsMessage.Parallel_ID = (byte)getNextLong(_commandBuffer, index);          //28
      
    return true;
  }
}

bool onPIGSn() //^P007PGSn<cr>: Device general status parameters inquiry
{
  if (_commandBuffer.length() < 60)
  {
    return false;
  }
  else
  {
    _qRaw.QPIGSn = _commandBuffer; //^P007PGSn  General Status
    int index = 5; //after the starting '^D113'
      _qpigsnMessage.Parallel_ID = (byte)getNextLong(_commandBuffer, index);          //A
      _qpigsnMessage.workMode = (byte)getNextLong(_commandBuffer, index);             //B
      _qpigsnMessage.faultCode = (byte)getNextLong(_commandBuffer, index);            //CC
      _qpigsnMessage.gridV = (short)getNextLong(_commandBuffer, index)*0.1;           //DDDD
      _qpigsnMessage.gridHz = (short)getNextLong(_commandBuffer, index)*0.1;          //EEE
      _qpigsnMessage.acOutV = (short)getNextLong(_commandBuffer, index)*0.1;          //FFFF
      _qpigsnMessage.acOutHz = (short)getNextLong(_commandBuffer, index)*0.1;         //GGG
      _qpigsnMessage.acOutVa = (short)getNextLong(_commandBuffer, index);             //HHHH
      _qpigsnMessage.acOutW = (short)getNextLong(_commandBuffer, index);              //IIII
      _qpigsnMessage.totalAcOutVa = (byte)getNextLong(_commandBuffer, index);         //JJJJJ
      _qpigsnMessage.totalAcOutW = (byte)getNextLong(_commandBuffer, index);          //KKKKK
      _qpigsnMessage.acOutPercent = (short)getNextLong(_commandBuffer, index)*0.1;    //LLL
      _qpigsnMessage.totAcOutPercent = (short)getNextLong(_commandBuffer, index)*0.1; //MMM
      _qpigsnMessage.battV = (short)getNextLong(_commandBuffer, index)*0.1;           //NNN
      _qpigsnMessage.battDischargeA = (short)getNextLong(_commandBuffer, index);      //OOO
      _qpigsnMessage.battChargeA = (short)getNextLong(_commandBuffer, index);         //PPP
      _qpigsnMessage.totBatChargeA = (short)getNextLong(_commandBuffer, index);       //QQQ
      _qpigsnMessage.batCapacity = (short)getNextLong(_commandBuffer, index);         //MMM
      _qpigsnMessage.solarW_PV1 = (short)getNextLong(_commandBuffer, index);          //RRRR
      _qpigsnMessage.solarW_PV2 = (short)getNextLong(_commandBuffer, index);          //SSSS
      _qpigsnMessage.solarV_PV1 = (short)getNextLong(_commandBuffer, index)*0.1;      //TTTT
      _qpigsnMessage.solarV_PV2 = (short)getNextLong(_commandBuffer, index)*0.1;      //UUUU
      _qpigsnMessage.MPPT1_Status = (byte)getNextLong(_commandBuffer, index);         //V
      _qpigsnMessage.MPPT2_Status = (byte)getNextLong(_commandBuffer, index);         //W
      _qpigsnMessage.LoadConn = (byte)getNextLong(_commandBuffer, index);             //X
      _qpigsnMessage.battDirection = (byte)getNextLong(_commandBuffer, index);        //Y
      _qpigsnMessage.DC_AC_Direction = (byte)getNextLong(_commandBuffer, index);      //Z
      _qpigsnMessage.LineDirection = (byte)getNextLong(_commandBuffer, index);        //a
      _qpigsnMessage.maxTemp = (byte)getNextLong(_commandBuffer, index);              //bbb
      
    return true;
  }
}




bool onPIRI() //QPIRI<cr>: Device Rating Information inquiry
{
  //if(_commandBuffer[1]=='N') _qAv.QPIRI = false; //deactivate if NAK
  if (_commandBuffer.length() < 45)
  {
    return false;
  }
  else
  {
    _qRaw.QPIRI = _commandBuffer;
    int index = 5; //after the starting '^D085'
    _qpiriMessage.gridRatingV = getNextFloat(_commandBuffer, index)*0.1; //AAAA
    _qpiriMessage.gridRatingA = getNextFloat(_commandBuffer, index)*0.1; //BBB
    _qpiriMessage.acOutRatingV = getNextFloat(_commandBuffer, index)*0.1; //CCCC
    _qpiriMessage.acOutRatingHz = getNextFloat(_commandBuffer, index)*0.1; //DDD
    _qpiriMessage.acOutRatingA = getNextFloat(_commandBuffer, index)*0.1; //EEE
    _qpiriMessage.acOutRatungVA = getNextFloat(_commandBuffer, index); //FFFF
    _qpiriMessage.acOutRatingW = getNextFloat(_commandBuffer, index); //GGGG
    _qpiriMessage.battRatingV = getNextFloat(_commandBuffer, index)*0.1; //HHH
    _qpiriMessage.battreChargeV = getNextFloat(_commandBuffer, index)*0.1; //III
    _qpiriMessage.battDischargeV = getNextFloat(_commandBuffer, index)*0.1; //JJJ
    _qpiriMessage.battUnderV = getNextFloat(_commandBuffer, index)*0.1; //KKK
    _qpiriMessage.battBulkV = getNextFloat(_commandBuffer, index)*0.1; //LLL
    _qpiriMessage.battFloatV = getNextFloat(_commandBuffer, index)*0.1; //MMM
   /* 
    switch ((byte)getNextLong(_commandBuffer, index))//N
    {
    case 0: _qpiriMessage.battType = "AGM"; break;
    case 1: _qpiriMessage.battType = "Flooded"; break;
    case 2: _qpiriMessage.battType = "User"; break;
    }

    */
    
    _qpiriMessage.battType = getNextFloat(_commandBuffer, index); //N
    _qpiriMessage.battMaxAcChrgA = getNextFloat(_commandBuffer, index); //OO
    _qpiriMessage.battMaxChrgA = getNextFloat(_commandBuffer, index); //PPP
    _qpiriMessage.imputVrange = getNextFloat(_commandBuffer, index); //Q
    _qpiriMessage.outSourcePrior = getNextFloat(_commandBuffer, index); //R
    _qpiriMessage.chargerSourcePrior = getNextFloat(_commandBuffer, index); //S
    _qpiriMessage.parallelMax = getNextFloat(_commandBuffer, index); //T
    _qpiriMessage.MachineType = getNextFloat(_commandBuffer, index); //U
    _qpiriMessage.topology = getNextFloat(_commandBuffer, index); //V
    _qpiriMessage.outModelSett = getNextFloat(_commandBuffer, index); //W
    _qpiriMessage.SolarPowPrior = getNextFloat(_commandBuffer, index); //Z
    _qpiriMessage.mpptString = getNextFloat(_commandBuffer, index); //a



    return true;
  }
}

bool onMOD() //QMOD<cr>: Device Mode inquiry
{
  // if(_commandBuffer[1]=='N') _qAv.QMOD = false; //deactivate if NAK
  if (_commandBuffer.length() < 2)
  {
    return false;
  }
  else
  {
    _qRaw.QMOD = _commandBuffer;
    int index = 5; //after the starting '^D005'
    _qmodMessage.operationMode_RAW = (byte)getNextLong(_commandBuffer, index);  //XX  
    switch (_qmodMessage.operationMode_RAW) //XX
    {
    case 0: _qmodMessage.operationMode = "Power On"; break;
    case 1: _qmodMessage.operationMode = "Standby"; break;
    case 2: _qmodMessage.operationMode = "Bypass"; break;
    case 3: _qmodMessage.operationMode = "Battery"; break;
    case 4: _qmodMessage.operationMode = "Fault"; break;
    case 5: _qmodMessage.operationMode = "Hybrid"; break;
    }
 /*   
    _qmodMessage.mode = _commandBuffer[1];    
    switch (_commandBuffer[1])
    {
    default:
      _qmodMessage.operationMode = "Undefined, Origin: " + _commandBuffer[1];
      break;
    case 'P':
      _qmodMessage.operationMode = "Power On";
      break;
    case 'S':
      _qmodMessage.operationMode = "Standby";
      break;
    case 'Y':
      _qmodMessage.operationMode = "Bypass";
      break;
    case 'L':
      _qmodMessage.operationMode = "Line";
      break;
    case 'B':
      _qmodMessage.operationMode = "Battery";
      break;
    case 'T':
      _qmodMessage.operationMode = "Battery Test";
      break;
    case 'F':
      _qmodMessage.operationMode = "Fault";
      break;
    case 'D':
      _qmodMessage.operationMode = "Shutdown";
      break;
    case 'G':
      _qmodMessage.operationMode = "Grid";
      break;
    case 'C':
      _qmodMessage.operationMode = "Charge";
      break;
    }
   */ 
    return true;
  }
}


bool onPIWS() //QPIWS<cr>: Device Warning Status inquiry
{
  //if(_commandBuffer[1]=='N') _qAv.QPIWS = false; //deactivate if NAK
  if (_commandBuffer.length() < 32)
  {
    return false;
  }
  else
  {
    _qRaw.QPIWS = _commandBuffer;
    int index = 5; //after the starting '('
    _qpiwsMessage.faultCode = getNextBit(_commandBuffer, index);//  AA
    _qpiwsMessage.lineFail = getNextBit(_commandBuffer, index);//B
    _qpiwsMessage.outCircShort = getNextBit(_commandBuffer, index);// C
    _qpiwsMessage.overTemperature = getNextBit(_commandBuffer, index);// D
    _qpiwsMessage.fanLocked = getNextBit(_commandBuffer, index);// E
    _qpiwsMessage.batteryVoltageHigh = getNextBit(_commandBuffer, index);// F
    _qpiwsMessage.inverterVoltLow = getNextBit(_commandBuffer, index);// G
    _qpiwsMessage.batteryUnderShutdown = getNextBit(_commandBuffer, index);// H
    _qpiwsMessage.overload = getNextBit(_commandBuffer, index);// I
    _qpiwsMessage.eepromFault = getNextBit(_commandBuffer, index);// J
    _qpiwsMessage.powerLimit = getNextBit(_commandBuffer, index);// K
    _qpiwsMessage.pvVoltageHigh = getNextBit(_commandBuffer, index);// L
    _qpiwsMessage.pv2VoltageHigh = getNextBit(_commandBuffer, index);// M
    _qpiwsMessage.mpptOverloadFault = getNextBit(_commandBuffer, index);// N
    _qpiwsMessage.mppt2OverloadFault = getNextBit(_commandBuffer, index);// O
    _qpiwsMessage.batteryTooLow4SCC1 = getNextBit(_commandBuffer, index);// P
    _qpiwsMessage.batteryTooLow4SCC2 = getNextBit(_commandBuffer, index);// Q
    
    return true;
  }
}
//*****************************************************************************************
bool onFLAG() //QFLAG<cr>: Device flag status inquiry
{
  //if(_commandBuffer[1]=='N') _qAv.QFLAG = false; //deactivate if NAK
  if (_commandBuffer.length() < 10)
  {
    return false;
  }
  else
  {
    _qRaw.QFLAG = _commandBuffer;
    int index = 1; //after the starting '('
    _qflagMessage.disableBuzzer = getNextBit(_commandBuffer, index);
    _qflagMessage.enableOverloadBypass = getNextBit(_commandBuffer, index);
    _qflagMessage.enablePowerSaving = getNextBit(_commandBuffer, index);
    _qflagMessage.enableLcdEscape = getNextBit(_commandBuffer, index);
    _qflagMessage.enableOverloadRestart = getNextBit(_commandBuffer, index);
    _qflagMessage.enableOvertempRestart = getNextBit(_commandBuffer, index);
    _qflagMessage.enableBacklight = getNextBit(_commandBuffer, index);
    _qflagMessage.enablePrimarySourceInterruptedAlarm = getNextBit(_commandBuffer, index);
    _qflagMessage.enableFaultCodeRecording = getNextBit(_commandBuffer, index);

    return true;
  }
}

bool onID() //QID<cr>: The device ID inquiry
{
  //if(_commandBuffer[1]=='N') _qAv.QID = false; //deactivate if NAK
  if (_commandBuffer.length() < 15)
  {
    return false;
  }
  else
  {
    _qRaw.QID = _commandBuffer;
    //Discard the first '('
    _commandBuffer.substring(1).toCharArray(_qidMessage.id, sizeof(_qidMessage.id) - 1);

    return true;
  }
}

bool onT() //^P004T<cr>: The device Time inquiry
{
   if (_commandBuffer.length() < 10)
  {
    return false;
  }
  else
  {
    _qRaw.QT = _commandBuffer;
    //Discard the first '^P004'
    _commandBuffer.substring(5).toCharArray(_qtMessage.qt, sizeof(_qtMessage.qt));

    return true;
  }
}

bool onDAT() //^S018DAT<cr>: Device Protocol ID Inquiry
{
  
  if (_commandBuffer.length() < 1)
  {
    return false;
  }
  else
  {
    _qRaw.DAT = _commandBuffer;
    //Get number after '^'
    int index = 1;
    _qdatMessage.dt = (char)getNextLong(_commandBuffer, index);

    return true;
  }
}

bool onPI() //^P005PI<cr>: Device Protocol ID Inquiry
{
  //if(_commandBuffer[1]=='N') _qAv.QPI = false; //deactivate if NAK
  if (_commandBuffer.length() < 5)
  {
    return false;
  }
  else
  {
    _qRaw.QPI = _commandBuffer;
    //Get number after '(PI'
    int index = 5;
    _qpiMessage.protocolId = (byte)getNextLong(_commandBuffer, index);

    return true;
  }
}

bool sendCommand(String command)
{
  _commandBuffer = "";

  unsigned short crc = cal_crc_half((byte *)command.c_str(), command.length());

#ifdef SERIALDEBUG
  Serial1.print(F("Sent Command: "));
  Serial1.println(command);

#endif
  Serial.print(command);
  Serial.print((char)((crc >> 8) & 0xFF)); //ONLY CRC fo PCM/PIP
  Serial.print((char)((crc >> 0) & 0xFF)); //ONLY CRC fo PCM/PIP
  Serial.print("\r");


  _commandBuffer = Serial.readStringUntil('\n');

  unsigned short calculatedCrc = cal_crc_half((byte *)_commandBuffer.c_str(), _commandBuffer.length() - 2);
  unsigned short recievedCrc = ((unsigned short)_commandBuffer[_commandBuffer.length() - 2] << 8);

  //remove the CRC from recived command
  if (_commandBuffer[_commandBuffer.length() - 2] < 6)
  {
    _commandBuffer.remove(_commandBuffer.length() - 2);
  }
  else
  {
    _commandBuffer.remove(_commandBuffer.length() - 3);
  }

//for bugfix add a space at the end of commandstring
_commandBuffer.concat(" ");

#ifdef SERIALDEBUG
      Serial1.print(F("   Calc: "));
      Serial1.print(calculatedCrc, HEX);
      Serial1.print(F("   Rx: "));
      Serial1.println(recievedCrc, HEX);
      Serial1.print(F("   Recived: "));
      Serial1.println(_commandBuffer);
#endif

  if (calculatedCrc == recievedCrc)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void requestInverter(qCommand com)
{
  switch (com)
  {
  case qCommand::QPI: if(sendCommand("^P005PI")) onPI(); break; //vollständig
  case qCommand::QID: break;
  case qCommand::QVFW: break;
  case qCommand::QVFW2: break;
  case qCommand::QPIRI: if(sendCommand("^P007PIRI")) onPIRI(); break; //vervollständigen!
  case qCommand::QFLAG: break;
  case qCommand::QPIGS: if(sendCommand("^P005GS")) onPIGS(); break; //vervollständigen!
  case qCommand::QPIGSn: if(sendCommand("^P007GS1")) onPIGS(); break; //vervollständigen!
  case qCommand::QMOD: if(sendCommand("^P006MOD")) onMOD(); break; //vollständig
  case qCommand::QPIWS:if(sendCommand("^P005FWS")) onPIWS(); break; //vollständig break;
  case qCommand::QDI: break;
  case qCommand::QBOOT: break;
  case qCommand::QOPM: break;
  case qCommand::QT: if(sendCommand("^P004T")) onT(); break; //vollständig break;
  case qCommand::DAT: if(sendCommand("^S018DAT" + currentDate)) onDAT(); break;
  }
}
