// ****************************************************************
// FrSky telemetry
// Changes:
// 2013-05-21 by disq
//                                - softserial support, frees up a serial port also eliminates the need to use a ttl inverter
// April 2013 by QuadBow - works with 2.2
//                                - compatible with display FLD-02
//                                - improved speed (to be sent in knots)
//                                - optimized scheduler
//                                - counts arming time
//                                - displays numbers of satellites as fuel bar
//                                - instead of altitude the distance to home can be displayed
//                                - integration of VBAT and POWERMETER
// Date 20/09/2012
// Changes: V0.2.1: - make it work with 2.1 (shared dev)
// Date: 14/08/2012
// Changes: V0.2: - Byte stuffing added
//                - vBat will be send, if "#define FAS_100" is comment out
//                V0.1: - First release
// ****************************************************************

#if defined(TELEMETRY_FRSKY)

#define OPENTX_TELEMETRY // define if using OpenTx. Use special OpenTx calculations for altitude and gps speed. Comment out for FrSky Telemetry Display

#ifdef TELEMETRY_FRSKY_SOFTSERIAL_PIN

#include "SendOnlySoftwareSerial.h"

static SendOnlySoftwareSerial telemSerial(TELEMETRY_FRSKY_SOFTSERIAL_PIN, true, true);

#endif

  // Frame protocol
  #define Protocol_Header    0x5E
  #define Protocol_Tail      0x5E

  // Data Ids  (bp = before point; af = after point)
  // Official data IDs
  #define ID_GPS_Altitude_bp    0x01
  #define ID_GPS_Altitude_ap    0x09
  #define ID_Temperature1       0x02
  #define ID_RPM                0x03
  #define ID_Fuel_level         0x04
  #define ID_Temperature2       0x05
  #define ID_Volt               0x06
  #define ID_Altitude_bp        0x10
  #define ID_Altitude_ap        0x21
  #define ID_GPS_speed_bp       0x11
  #define ID_GPS_speed_ap       0x19
  #define ID_Longitude_bp       0x12
  #define ID_Longitude_ap       0x1A
  #define ID_E_W                0x22
  #define ID_Latitude_bp        0x13
  #define ID_Latitude_ap        0x1B
  #define ID_N_S                0x23
  #define ID_Course_bp          0x14
  #define ID_Course_ap          0x1C
  #define ID_Date_Month         0x15
  #define ID_Year               0x16
  #define ID_Hour_Minute        0x17
  #define ID_Second             0x18
  #define ID_Acc_X              0x24
  #define ID_Acc_Y              0x25
  #define ID_Acc_Z              0x26
  #define ID_Voltage_Amp_bp     0x3A
  #define ID_Voltage_Amp_ap     0x3B
  #define ID_Current            0x28
  // User defined data IDs
  #define ID_Gyro_X             0x40
  #define ID_Gyro_Y             0x41
  #define ID_Gyro_Z             0x42

   // Main function FrSky telemetry
    void inline telemetry_frsky()
   {
      static uint32_t lastTime;
      static uint8_t tele_loop;
      if ((millis() - lastTime) > 250) {
         // Data sent every 250ms
         lastTime = millis();
         tele_loop++;
//         send_Accel();

         // Data sent every 1s
         switch (tele_loop) {
            case 1:
               send_GPS_longitude();
               send_Altitude();
               send_Course();
               send_Accel();
            break;
            case 2:
               send_RPM();
               send_GPS_speed();
               send_Voltage_ampere();
            break;
            case 3:
               send_Num_Sat();
               send_GPS_latitude();
//               send_Distance();
               send_GPS_altitude();
               send_Accel();
            break;
            case 4:
               send_Temperature();
               send_RPM();
               send_Time();
               send_Voltage_ampere();
               tele_loop = 0;
            break;
            default:
            break;
            }
        sendDataTail();
        }
   }

   void inline write_FrSky8(uint8_t Data)
   {
#ifdef TELEMETRY_FRSKY_SOFTSERIAL_PIN
      telemSerial.write(Data);
#else
      SerialWrite(TELEMETRY_FRSKY_SERIAL, Data);
#endif
   }

   void inline write_FrSky16(uint16_t Data)
   {
      uint8_t Data_send;
      Data_send = Data;
      check_FrSky_stuffing(Data_send);
      Data_send = Data >> 8 & 0xff;
      check_FrSky_stuffing(Data_send);
   }

   void inline check_FrSky_stuffing(uint8_t Data) //byte stuffing
   {
      if (Data == 0x5E)
      {
         write_FrSky8(0x5D);
         write_FrSky8(0x3E);
      }
      else if (Data == 0x5D)
      {
         write_FrSky8(0x5D);
         write_FrSky8(0x3D);
      }
      else
      {
         write_FrSky8(Data);
      }
   }

   static void inline sendDataHead(uint8_t Data_id)
   {
      write_FrSky8(Protocol_Header);
      write_FrSky8(Data_id);
   }

   static void inline sendDataTail(void)
   {
      write_FrSky8(Protocol_Tail);
   }

   static void inline sendTwoPart(uint8_t bpId, uint8_t apId, float value, uint16_t resolution = 100)
   {
         int16_t bpVal;
         uint16_t apVal;

         bpVal = floor(value); // value before the decimal point ("bp" is "before point")
         apVal = (value - int(value)) * resolution; // value after the decimal point

         sendDataHead(bpId);
         write_FrSky16(bpVal);
         sendDataHead(apId);
         write_FrSky16(apVal);
   }

   //*********************************************************************************
   //-----------------   Telemetry Data   -----------------------------------------
   //*********************************************************************************

   // GPS altitude
   void inline send_GPS_altitude(void)
   {
      if (f.GPS_FIX && GPS_numSat >= 4) sendTwoPart(ID_GPS_Altitude_bp, ID_GPS_Altitude_ap, GPS_altitude);
   }

   // Temperature
   void inline send_Temperature(void)
   {
#if defined(BARO)
      int16_t Data_Temperature1;

      Data_Temperature1 = baroTemperature / 100;
      sendDataHead(ID_Temperature1);
      write_FrSky16(Data_Temperature1);
//      sendDataHead(ID_Temperature2);
//      write_FrSky16(0);
#endif
   }

   // RPM
   void inline send_RPM(void)
   {
#if defined(KV_MOTOR)
      uint16_t Data_RPM = 0;

      Data_RPM = rpm;

      sendDataHead(ID_RPM);
      write_FrSky16(Data_RPM);
#endif
   }

   // Fuel level
   void inline send_Num_Sat(void)
   {
      uint16_t Data_Num_Sat;

      Data_Num_Sat = (GPS_numSat / 2) * 25;

      sendDataHead(ID_Fuel_level);
      write_FrSky16(Data_Num_Sat);
   }

   // Temperature 2
   void inline send_Distance(void)
   {
      if (f.GPS_FIX_HOME)
      {
      sendDataHead(ID_Temperature2);
      write_FrSky16(GPS_distanceToHome); // Distance to home alias Temp2
      }
   }

   // Cell voltage  todo !!!!!!!!!!!!!!!!!!
   void inline send_Cell_volt(void) // Data FrSky FLVS-01 voltage sensor
   {
      uint16_t Data_Volt;
      uint8_t number_of_cells = 0;   // LiPo 3S = 3; LiPo 4S = 4 ...
      static uint8_t cell = 0;

      if (cell >= number_of_cells)
         cell = 0;
      Data_Volt = 0; // 0.01v / 0 ~ 4.2v

      sendDataHead(ID_Volt);
      write_FrSky16(Data_Volt);
   }

   // Altitude
   void inline send_Altitude(void)
   {
#if defined(BARO)
#if defined(OPENTX_TELEMETRY)
         sendDataHead(ID_Altitude_bp);
         write_FrSky16(round(EstAlt / 100) - 677 /*Arbitrary constant tested with OpenTx r2324*/);
// ID_Altitude_ap doesn't work/unused with OpenTx
#else
      sendTwoPart(ID_Altitude_bp, ID_Altitude_ap, EstAlt / 100);
#endif
#endif
   }

   // GPS speed
   void inline send_GPS_speed(void)
   {
      if (f.GPS_FIX && GPS_numSat >= 4)
      {
#if defined(OPENTX_TELEMETRY)
        sendTwoPart(ID_GPS_speed_bp, ID_GPS_speed_ap, GPS_speed * 0.036 /*cm/s to km/h*/ * 0.5449 /* Arbitrary constant tested with OpenTx r2324 */ );
#else
        sendTwoPart(ID_GPS_speed_bp, ID_GPS_speed_ap, GPS_speed * 0.0194384449); // cm/s to knots
#endif
      }
   }

   // GPS position
 void inline send_GPS_longitude(void)
      {
      uint16_t Data_Longitude_bp;
      uint16_t Data_Longitude_ap;
      uint16_t Data_E_W;

      if (f.GPS_FIX && GPS_numSat >= 4)
         {
         float lon = fabs(GPS_coord[LON] / 10000000.0f * 100);
         Data_Longitude_bp = lon;
         Data_Longitude_ap = (lon-int(lon))*10000;
         Data_E_W = GPS_coord[LON] < 0 ? 'W' : 'E';

         sendDataHead(ID_Longitude_bp);
         write_FrSky16(Data_Longitude_bp);
         sendDataHead(ID_Longitude_ap);
         write_FrSky16(Data_Longitude_ap);
         sendDataHead(ID_E_W);
         write_FrSky16(Data_E_W);
         }
      }

 void inline send_GPS_latitude(void)
      {
      uint16_t Data_Latitude_bp;
      uint16_t Data_Latitude_ap;
      uint16_t Data_N_S;

      if (f.GPS_FIX && GPS_numSat >= 4)
         {
         float lat = fabs(GPS_coord[LAT] / 10000000.0f * 100);
         Data_Latitude_bp = lat;
         Data_Latitude_ap = (lat-int(lat))*10000;
         Data_N_S = GPS_coord[LAT] < 0 ? 'S' : 'N';

         sendDataHead(ID_Latitude_bp);
         write_FrSky16(Data_Latitude_bp);
         sendDataHead(ID_Latitude_ap);
         write_FrSky16(Data_Latitude_ap);
         sendDataHead(ID_N_S);
         write_FrSky16(Data_N_S);
         }
   }

   // Course
   void inline send_Course(void)
   {
         sendDataHead(ID_Course_bp);
         write_FrSky16(heading<0 ? heading+360 : heading);
   }

   // Time
   void inline send_Time(void)
   {
      uint16_t seconds_since_start;
      uint16_t Data_Minutes_hours;
      uint16_t Data_seconds;

      if (showTime.TimerStart) {
         seconds_since_start = (millis() - showTime.armingTime) / 1000;
         Data_Minutes_hours = seconds_since_start / 60;
         Data_seconds = seconds_since_start - 60 * Data_Minutes_hours;
         sendDataHead(ID_Hour_Minute);
         write_FrSky16(Data_Minutes_hours * 256);
         sendDataHead(ID_Second);
         write_FrSky16(Data_seconds);
         }
   }

   // ACC
   void inline send_Accel(void)
   {
      int16_t Data_Acc_X;
      int16_t Data_Acc_Y;
      int16_t Data_Acc_Z;

      Data_Acc_X = ((float)accSmooth[0] / acc_1G) * 1000;
      Data_Acc_Y = ((float)accSmooth[1] / acc_1G) * 1000;
      Data_Acc_Z = ((float)accSmooth[2] / acc_1G) * 1000;

      sendDataHead(ID_Acc_X);
      write_FrSky16(Data_Acc_X);
      sendDataHead(ID_Acc_Y);
      write_FrSky16(Data_Acc_Y);
      sendDataHead(ID_Acc_Z);
      write_FrSky16(Data_Acc_Z);
   }

   // Voltage (Ampere Sensor)
   void inline send_Voltage_ampere(void)
   {
#if defined(VBAT)
      uint16_t Data_Voltage_vBat_bp;
      uint16_t Data_Voltage_vBat_ap;
      uint16_t Data_Voltage_I_Motor;

     uint16_t volts = vbat * 110 / 21; // tested with OpenTx
     Data_Voltage_vBat_bp = volts / 100;
     Data_Voltage_vBat_ap = (volts % 100) / 10;

      sendDataHead(ID_Voltage_Amp_bp);
      write_FrSky16(Data_Voltage_vBat_bp);
      sendDataHead(ID_Voltage_Amp_ap);
      write_FrSky16(Data_Voltage_vBat_ap);

      #if defined(POWERMETER)
        Data_Voltage_I_Motor = pCurrent / 10;
        sendDataHead(ID_Current);
        write_FrSky16(Data_Voltage_I_Motor);
      #endif
#endif
   }

void init_telemetry()
{
#if defined(TELEMETRY_FRSKY_SOFTSERIAL_PIN)
  telemSerial.begin(TELEMETRY_FRSKY_SERIAL);
#endif
}

#endif

