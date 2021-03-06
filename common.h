/*
    @file   -   common.h
    @author -   
    @brief  -   manage shared variable in thread.
    @reference - 
*/
#include <iostream>
#include <pthread.h>
using namespace std;

#ifndef SYSPROG_COMMON_H
#define SYSPROG_COMMON_H

#define NOT_AUTHORIZED -1
#define OBSERVE_MODE 1
#define EDIT_MODE 2

class Data {
public:
    int illumination, temperature, humidity, soil_humidity;
    Data(){
        illumination = 0;
        temperature = 0;
        humidity = 0;
        soil_humidity = 0;
    }
    Data(int illumination, int temperature, int humidity, int soil_humidity) {
        this->illumination = illumination;
        this->temperature = temperature;
        this->humidity = humidity;
        this->soil_humidity = soil_humidity;
    }
};

class State {
public:
    int state[10];
    int len;

    State() {
        this->len = 0;
    }
};

class Shared {
public:
    Data data;///< standard data.
    Data sensor;///< sensor data.
    State state;///< 
    int mode;
    int cledValue[4];
    std::string id;
    bool liq_exist;
    bool sns_exist; 
    int segValue;

    int getCompare() {
        int cnt = 0;
        if( sensor.illumination >= data.illumination ) cnt++;
        if( sensor.temperature >= data.temperature ) cnt++;
        if( sensor.humidity >= data.humidity ) cnt++;
        if( sensor.soil_humidity <= data.soil_humidity ) cnt++;

        return cnt;
    }
    Shared(){
        for(int i = 0 ; i < 4 ; i++)
        {
            cledValue[i] = 0;
        }
        sns_exist = false;
        liq_exist = false;
        segValue = -1;
        mode = -1;
        this->mode = -1;
    }
};


namespace edit{
    extern void init();
    extern void* edit(void* shared);
};

namespace observe{
    extern void init();
    extern void* observe(void* shared);
};

namespace auth{
    extern void input_account(Shared* s);
    extern int authorize(string id, string* result);
}

#endif //SYSPROG_COMMON_H
