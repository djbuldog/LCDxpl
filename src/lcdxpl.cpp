#include <cstring>
#include <vector>
#include "XPLMProcessing.h"

#include "serial.h"
#include "datarefs.h"

/* Global variables */
static std::vector<DataRef*> datarefs;
static Serial ser;
static float callback_rate = -1.0;

float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {

	ser.open("/dev/ttyACM1");

	strcpy(outName, "LCDxpl");
	strcpy(outSig, "bari2.eu");
	strcpy(outDesc, "A plugin for X-Plane LCD.");

	datarefs.push_back(new DataRefInt("sim/cockpit2/radios/actuators/transponder_code","SQK"));
	datarefs.push_back(new DataRefInt("sim/cockpit2/radios/actuators/adf1_frequency_hz","ADF"));

	datarefs.push_back(new DataRefIntInt("sim/cockpit2/radios/actuators/com1_frequency_hz","C1u"));
	datarefs.push_back(new DataRefIntInt("sim/cockpit2/radios/actuators/com1_standby_frequency_hz","C1s"));
	datarefs.push_back(new DataRefIntInt("sim/cockpit2/radios/actuators/nav1_frequency_hz","N1u"));
	datarefs.push_back(new DataRefIntInt("sim/cockpit2/radios/actuators/nav1_standby_frequency_hz","N1s"));
	
	for(std::vector<DataRef*>::iterator it = datarefs.begin(); it != datarefs.end(); ++it) {
		if (!(*it)->isInit()) {
			std::cerr << "LCDxpl: Some dataref obj is not initialized!!" << std::endl;
			return 0;
		}
	}

	XPLMRegisterFlightLoopCallback(FlightLoopCallback,callback_rate,NULL);

	return 1;
}

PLUGIN_API void	XPluginStop(void) {

  XPLMUnregisterFlightLoopCallback(FlightLoopCallback, NULL);
  
	for(std::vector<DataRef*>::iterator it = datarefs.begin(); it != datarefs.end(); ++it) {
    delete *it;
	}
	datarefs.clear();
	
	ser.close();

}

PLUGIN_API void XPluginDisable(void) {
}

PLUGIN_API int XPluginEnable(void) {
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void *inParam) {
}

float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon) {

	for(std::vector<DataRef*>::iterator it = datarefs.begin(); it != datarefs.end(); ++it) {
		(*it)->update();
		if ((*it)->isChanged()) {
			ser.write((*it)->getSerStr());
		}
	}

	return callback_rate;

}
