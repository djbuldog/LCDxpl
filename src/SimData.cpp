#include <cstdlib>
#include <string.h>
#include <sstream>
#include <vector>
#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

#include "serial.h"

class DataRefOld {
	int last;

	protected:
		bool changed;
		XPLMDataRef ptr;
		std::string ser_code;
		const std::string getValStr();
		
	public:
		DataRefOld(): changed(false), ptr(NULL) {}
		DataRefOld(const std::string name, const std::string ser_code): last(0) { assign(name, ser_code); }
		void assign(const std::string name, const std::string ser_code);
		bool isChanged() { return changed; };
		bool isInit() { return (ptr!=NULL); }
		void update();
		const std::string getSerStr();
		
};

/* ******************************************************************* */

void DataRefOld::assign(const std::string name, const std::string ser_code) {
	this->ser_code = ser_code;
	ptr = XPLMFindDataRef(name.c_str());
	changed = false;
}

const std::string DataRefOld::getSerStr() {
	this->changed = false;
	std::string retval(ser_code);
	return retval + " " + getValStr() + '\n';
}

void DataRefOld::update() {

	if (ptr == NULL) return;

	int val = XPLMGetDatai(this->ptr);
	if (val!=this->last) {
		changed = true;
		last=val;
	}  
}

const std::string DataRefOld::getValStr() {
	std::stringstream ss;
	ss << this->last;
  return ss.str();
}

/*
class DataRef {

	protected:
		bool changed;
		XPLMDataRef ptr;
		std::string ser_code;
		virtual const std::string getValStr();
		
	public:
		DataRef(): changed(false), ptr(NULL) {}
		void assign(const std::string name, const std::string ser_code);
		bool isChanged() { return changed; };
		bool isInit() { return (ptr!=NULL); }
		virtual void update();
		const std::string getSerStr();
		
};

class DataRefInt : public DataRef {
	int last;

	protected:
		const std::string getValStr();
  
	public:
		DataRefInt(): last(0), DataRef() {}
		DataRefInt(const std::string name, const std::string ser_code): DataRef(), last(0) { assign(name, ser_code); }
		void update();
		const std::string getSerStr();
};

class DataRefFloat : public DataRef {
	float last;

	protected:
		const std::string getValStr();
  
	public:
		DataRefFloat(): last(0.0), DataRef() {}
		void update();
		const std::string getSerStr();
};

*/
/* ******************************************************************* */
/*
void DataRef::assign(const std::string name, const std::string ser_code) {
	this->ser_code = ser_code;
	ptr = XPLMFindDataRef(name.c_str());
	changed = false;
}

const std::string DataRef::getSerStr() {
	this->changed = false;
	std::string retval(ser_code);
	return retval + " " + getValStr();
}
*/
/* ******************************************************************* */
/*
void DataRefInt::update() {

	if (ptr == NULL) return;

	int val = XPLMGetDatai(this->ptr);
	if (val!=this->last) {
		changed = true;
		last=val;
	}  
}

const std::string DataRefInt::getValStr() {
	std::stringstream ss;
	ss << this->last;
  return ss.str();
}
*/
/* ******************************************************************* */
/*
void DataRefFloat::update() {

	if (ptr == NULL) return;

	float val = XPLMGetDatai(this->ptr);
	if (val!=this->last) {
		changed = true;
		last=val;
	}  
}

const std::string DataRefFloat::getValStr() {
	std::stringstream ss;
	ss << this->last;
  return ss.str();
}
*/
/* ******************************************************************* */

/* We keep our data ref globally since only one is used for the whole plugin. */
//static std::vector<DataRef> datarefs;
static std::vector<DataRefOld> datarefs;
static serial ser;
static float callback_rate = -1.0;

float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {

	ser.open("/dev/ttyACM1");

	strcpy(outName, "LCDxpl");
	strcpy(outSig, "bari2.eu");
	strcpy(outDesc, "A plugin for X-Plane LCD.");
/*
	datarefs.push_back(DataRefInt("sim/cockpit2/radios/actuators/transponder_code","SQK"));
	datarefs.push_back(DataRefInt("sim/cockpit2/radios/actuators/adf1_frequency_hz","ADF"));
*/	
	datarefs.push_back(DataRefOld("sim/cockpit2/radios/actuators/transponder_code","SQK"));
	datarefs.push_back(DataRefOld("sim/cockpit2/radios/actuators/adf1_frequency_hz","ADF"));

	XPLMRegisterFlightLoopCallback(FlightLoopCallback,callback_rate,NULL);

	/* Only return that we initialized correctly if we found the data ref. */
/*	
	for(std::vector<DataRef>::iterator it = datarefs.begin(); it != datarefs.end(); ++it) {
		if (!it->isInit()) return 0;
	}
*/
	for(std::vector<DataRefOld>::iterator it = datarefs.begin(); it != datarefs.end(); ++it) {
		if (!it->isInit()) return 0;
	}

	return 1;
}

PLUGIN_API void	XPluginStop(void) {
  XPLMUnregisterFlightLoopCallback(FlightLoopCallback, NULL);
}

PLUGIN_API void XPluginDisable(void) {
}

PLUGIN_API int XPluginEnable(void) {
	return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void *inParam) {
}

float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon) {
/*
	for(std::vector<DataRef>::iterator it = datarefs.begin(); it != datarefs.end(); ++it) {
		it->update();
		if (it->isChanged()) {
			ser.write(it->getSerStr());
		}
	}
*/

	for(std::vector<DataRefOld>::iterator it = datarefs.begin(); it != datarefs.end(); ++it) {
		it->update();
		if (it->isChanged()) {
			ser.write(it->getSerStr());
		}
	}

	return callback_rate;

}
